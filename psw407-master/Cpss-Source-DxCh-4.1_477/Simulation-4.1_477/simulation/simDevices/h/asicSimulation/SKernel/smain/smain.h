/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smain.h
*
* DESCRIPTION:
*       This is a external API definition for Smain module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 335 $
*
*******************************************************************************/
#ifndef __smainh
#define __smainh

#include <common/SBUF/sbuf.h>
#include <common/SQue/squeue.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <common/Utils/PresteraHash/smacHash.h>
#include <common/Utils/CRC/simCrc.h>

/***********************************************/

/* simulation bookmark*/
#define SIM_TBD_BOOKMARK

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* enum removed to avoid inconsistency and compilation warnings with corresponding
 * fields in different type structures where device_type is GT_U32
 */
#define WHITE_MODEL             0
#define GOLDEN_MODEL            1
#define TIGER_GOLDEN_MODEL_XII  2
#define TIGER_GOLDEN_MODEL_MII  3

#define INI_FILE_SYSTEM_SECTION_CNS "system"

/* not valid value */
#define SMAIN_NOT_VALID_CNS 0xFFFFFFFF

#ifndef _1K
    #define _1K         1024
#endif /* not defined _1K */

/* max number of ports in cheetah devices (supporting any future device) */
#define SKERNEL_CHEETAH_EGRESS_MAX_PORT_CNS                      256

/* bus id multiplier */
#define SMAIN_BUS_ID_MULTIPLIER_CNS     10

/* convert bit to table entry word number */
#define SMAIN_BIT_TO_REG_INDEX_MAC(bit)     ((bit) >> 5)

/* convert bit to index of bit in table entry word */
#define SMAIN_BIT_TO_INDEX_IN_REG_MAC(bit)     ((bit) & 0x1f)

/* set index of bit in bmp array */
#define SMAIN_INDEX_IN_BMP_ARRAY_SET_MAC(bmpArrPtr,index)   \
    bmpArrPtr[(index)>>5] |= 1 << ((index)&0x1f)

/* Get value of index of bit in bmp array */
#define SMAIN_INDEX_IN_BMP_ARRAY_GET_MAC(bmpArrPtr,index)   \
    ((bmpArrPtr[(index)>>5] & (1 << ((index)&0x1f))) ? 1 : 0)

/* clear index of bit in bmp array */
#define SMAIN_INDEX_IN_BMP_ARRAY_CLEAR_MAC(bmpArrPtr,index)   \
    bmpArrPtr[(index)>>5] &= ~(1 << ((index)&0x1f))

/* add 'case' to switch-case to allow assign on the name of the 'name' */
#define SWITCH_CASE_NAME_MAC(/*IN*/name,/*OUT*/nameStr) \
    case (name):                                        \
        nameStr = #name;                                \
        break
/* set parameter with new value only if current value is 0 */
#define SET_IF_ZERO_MAC(to_param , from_param)  \
    if ((to_param) == 0) to_param = from_param

/* set parameter with new value only if condition 'true' (non-zero) */
#define SET_IF_CONDITION_TRUE_MAC(condition , to_param , from_param)  \
    if (condition) to_param = from_param

/* number of DP (data path) that the device support */
#define MAX_DP_CNS  6

/* frame id array size */
#define FRAME_ID_SIZE 255

/* number of traffic classes */
#define NUM_OF_TRAFFIC_CLASSES 8

/* sip5 : FDB number of bunks */
#define FDB_MAX_BANKS_CNS   16

/* timestamp queues */
#define MAX_TIMESTAMP_QUEUE_LENGTH_CNS 256
#define TIMESTAMP_QUEUE_WIDTH_CNS 3
#define TIMESTAMP_QUEUE_NUM_CNS 2

/* cookie info that bound to 'thread/task' */
typedef struct{
    SIM_OS_TASK_COOKIE_INFO_STC         generic;/* must be first for casting */
    struct SIM_LOG_FRAME_INFO_STCT      *logInfoPtr;/* log info that is used for packet processing
                                                memory dynamic allocated by the LOGGER engine */
    /* add here other info needed to be save 'per thread/task' */
}SKERNEL_TASK_COOKIE_INFO_STC;

/*
 * Linked-list of packet descriptors:
 *
 *   descrPtr     - current packet descriptor;
 *   egressPort   - the egress port that associated with the descriptor
 *   destVlanTagged - the egress tag state
 *   nextDescrPtr - next packet descriptor in queue. The NULL value mean end of the queue.
 */
typedef struct SIM_TRANSMIT_QUEUE_STCT{
    struct SKERNEL_FRAME_CHEETAH_DESCR_STCT *descrPtr;
    GT_U32  egressPort;
    GT_U32  destVlanTagged;
    struct SIM_TRANSMIT_QUEUE_STCT          *nextDescrPtr;
} SIM_TRANSMIT_QUEUE_STC;

/*
 * Typedef: enum CHT_PACKET_CLASSIFICATION_TYPE_ENT
 *
 * Description: Types of packets - the values as indexes of UDB templates
 *
 * Enumerations:
 *    CHT_PACKET_CLASSIFICATION_TYPE_IPV4_TCP_E   - IPV4 TCP not fragmented packet
 *    CHT_PACKET_CLASSIFICATION_TYPE_IPV4_UDP_E   - IPV4 UDP not fragmented packet
 *    CHT_PACKET_CLASSIFICATION_TYPE_MPLS_E       - MPLS packet
 *    CHT_PACKET_CLASSIFICATION_TYPE_IPV4_FRAG_E  - IPV4 Fragmented packet,
 *               The packet is an initial fragment (IPv4 <More Fragment> flag is set),
 *               OR,the packet is a non-initial fragment
 *               (IPv4 <Fragment Offset> field is non-zero).
 *    CHT_PACKET_CLASSIFICATION_TYPE_IPV4_OTHER_E - IPV4 Other packet
 *               (not TCP, not UDP and not fragmented)
 *    CHT_PACKET_CLASSIFICATION_TYPE_ETH_OTHER_E  - all other types
 *               (not IPv4, not IPv6 and not MPLS)
 *    CHT_PACKET_CLASSIFICATION_TYPE_UDE0_E       - User Defined EtherType 0
 *    CHT_PACKET_CLASSIFICATION_TYPE_IPV6_E       - IPV6 packet
 *    CHT_PACKET_CLASSIFICATION_TYPE_UDE1_E       - User Defined EtherType 1
 *    CHT_PACKET_CLASSIFICATION_TYPE_UDE2_E       - User Defined EtherType 2
 *    CHT_PACKET_CLASSIFICATION_TYPE_UDE3_E       - User Defined EtherType 3
 *    CHT_PACKET_CLASSIFICATION_TYPE_UDE4_E       - User Defined EtherType 4
 *
 *
 */
typedef enum
{
    CHT_PACKET_CLASSIFICATION_TYPE_IPV4_TCP_E     = 0,
    CHT_PACKET_CLASSIFICATION_TYPE_IPV4_UDP_E     = 1,
    CHT_PACKET_CLASSIFICATION_TYPE_MPLS_E         = 2,
    CHT_PACKET_CLASSIFICATION_TYPE_IPV4_FRAG_E    = 3,
    CHT_PACKET_CLASSIFICATION_TYPE_IPV4_OTHER_E   = 4,
    CHT_PACKET_CLASSIFICATION_TYPE_ETH_OTHER_E    = 5,
    CHT_PACKET_CLASSIFICATION_TYPE_UDE0_E         = 6,
    CHT_PACKET_CLASSIFICATION_TYPE_IPV6_E         = 7,
    CHT_PACKET_CLASSIFICATION_TYPE_UDE1_E         = 8,
    CHT_PACKET_CLASSIFICATION_TYPE_UDE2_E         = 9,
    CHT_PACKET_CLASSIFICATION_TYPE_UDE3_E         = 10,
    CHT_PACKET_CLASSIFICATION_TYPE_UDE4_E         = 11,

} CHT_PACKET_CLASSIFICATION_TYPE_ENT;


typedef enum
{
    SKERNEL_MLL_SELECT_EXTERNAL_E = 0,
    SKERNEL_MLL_SELECT_INTERNAL_E
}SKERNEL_MLL_SELECT_ENT;

/*
 * Typedef: enum SIP5_PACKET_CLASSIFICATION_TYPE_ENT
 *
 * Description: SIP5 - Types of packets - the values as indexes of UDB templates
 *
 * Enumerations:
 *
 */
typedef enum
{
    SIP5_PACKET_CLASSIFICATION_TYPE_IPV4_TCP_E     ,
    SIP5_PACKET_CLASSIFICATION_TYPE_IPV4_UDP_E     ,
    SIP5_PACKET_CLASSIFICATION_TYPE_MPLS_E         ,
    SIP5_PACKET_CLASSIFICATION_TYPE_IPV4_FRAG_E    ,
    SIP5_PACKET_CLASSIFICATION_TYPE_IPV4_OTHER_E   ,
    SIP5_PACKET_CLASSIFICATION_TYPE_ETH_OTHER_E    ,
    SIP5_PACKET_CLASSIFICATION_TYPE_IPV6_E         ,
    SIP5_PACKET_CLASSIFICATION_TYPE_IPV6_TCP_E     ,
    SIP5_PACKET_CLASSIFICATION_TYPE_IPV6_UDP_E     ,
    SIP5_PACKET_CLASSIFICATION_TYPE_UDE0_E         ,
    SIP5_PACKET_CLASSIFICATION_TYPE_UDE1_E         ,
    SIP5_PACKET_CLASSIFICATION_TYPE_UDE2_E         ,
    SIP5_PACKET_CLASSIFICATION_TYPE_UDE3_E         ,
    SIP5_PACKET_CLASSIFICATION_TYPE_UDE4_E         ,
    SIP5_PACKET_CLASSIFICATION_TYPE_UDE5_E         ,
    SIP5_PACKET_CLASSIFICATION_TYPE_UDE6_E

} SIP5_PACKET_CLASSIFICATION_TYPE_ENT;



/* Device Type enumeration */
typedef enum
{
    SKERNEL_EMPTY = 0,  /* empty -- device not exists */
    /* Generic Device Type enumeration */
    SKERNEL_NIC = 1,
    SKERNEL_COM_MODULE, /* Communication module */
    SKERNEL_DXCH ,/* Cheetah +,HX */
    SKERNEL_DXCH_B0  ,/* Cheetah B0 (diamond cat) */
    SKERNEL_DXCH2 ,/* Cheetah2 */
    SKERNEL_DXCH3 ,/* Cheetah3 */
    SKERNEL_DXCH3_XG ,/* Cheetah3 XG device */
    SKERNEL_DXCH3P,/* Cheetah3+ */
    SKERNEL_EMBEDDED_CPU, /* embedded CPU */
    SKERNEL_XCAT_24_AND_4,/* xCat 24fe/ge+4stack*/
    SKERNEL_XCAT3_24_AND_6,/* xCat3 24ge+4stack(6 XG ports)+cpuPort(#31) */
    SKERNEL_XCAT2_24_AND_4,/* xCat2 24fe+4stack*/
    SKERNEL_LION_PORT_GROUP_12,/* Lion port group - 12 G/XG ports */
    SKERNEL_LION_48,/* Lion - 48 G/XG ports */

    SKERNEL_PHY_SHELL, /* the PHY shell (can support Quad/Octet PHY) that hold multi-cores */
    SKERNEL_PHY_CORE_1540M_1548M,   /* the core of PHY 1540M */

    SKERNEL_MACSEC,    /* the MACSEC device */

    SKERNEL_LION2_PORT_GROUP_12,/* Lion2 port group - 12 G/XG ports */
    SKERNEL_LION2_96,/* Lion2 - 96 G/XG ports */

    SKERNEL_LION3_PORT_GROUP_12,/* Lion3 port group - 12 G/XG ports */
    SKERNEL_LION3_96,/* Lion3 - 96 G/XG ports */

    SKERNEL_PUMA3_NETWORK_FABRIC,/* Puma3 network core fabric core - 32 ports (16+16) */
    SKERNEL_PUMA3_64,/* Puma3 64 G/XG ports */

    SKERNEL_BOBCAT2,/* Bobcat2 */
    SKERNEL_BOBK_CAELUM,/* BOBK - Caelum */
    SKERNEL_BOBK_CETUS,/* BOBK - Cetus */
    SKERNEL_BOBCAT3,/* Bobcat3 */
    /* Sapphire */
    SKERNEL_SAPPHIRE    = 0x1A30,
    /* Ruby */
    SKERNEL_RUBY        = 0x0930,
    /* Opal */
    SKERNEL_OPAL        = 0x0950,
    /* Opal Plus */
    SKERNEL_OPAL_PLUS   = 0x0990,
    /* Jade */
    SKERNEL_JADE        = 0x01A5,

    /* Salsa A1 */
    SKERNEL_98DX120     = 0x0D1611AB,
    SKERNEL_98DX121     = 0x0D1211AB,

    SKERNEL_98DX160     = 0x0D0511AB,
    SKERNEL_98DX161     = 0x0D0111AB,
    SKERNEL_98DX240     = 0x0D0411AB,
    SKERNEL_98DX241     = 0x0D0011AB,

    /* Salsa A2 */
    SKERNEL_98DX1602    = 0x0D1511AB,
    SKERNEL_98DX1612    = 0x0D1111AB,
    SKERNEL_98DX2402    = 0x0D1411AB,
    SKERNEL_98DX2412    = 0x0D1011AB,

    /* twist - D */
    SKERNEL_98EX100D    = 0x00EF11AB,
    SKERNEL_98EX110D    = 0x00E711AB,
    SKERNEL_98EX115D    = 0x00E111AB,
    SKERNEL_98EX110DS   = 0x00F711AB,
    SKERNEL_98EX115DS   = 0x00F111AB,
    SKERNEL_98EX120D    = 0x00D711AB,
    SKERNEL_98EX125D    = 0x00D111AB,
    SKERNEL_98EX130D    = 0x01D711AB,
    SKERNEL_98EX135D    = 0x01D111AB,

    /* samba */
    SKERNEL_98MX615D    = 0x01a011AB,
    SKERNEL_98MX625D    = 0x018011AB,
    SKERNEL_98MX625V0   = 0x019011AB,
    SKERNEL_98MX635D    = 0x01D011AB,

    /*rumba (Samba)*/
    SKERNEL_98MX635     = 0x01D011AB,
    SKERNEL_98MX618     = 0x00A011AB,
    SKERNEL_98MX628     = 0x009011AB,
    SKERNEL_98MX638     = 0x00D011AB,

    /* twist - C*/
    SKERNEL_98MX610B    = 0x007011AB,
    SKERNEL_98MX620B    = 0x004011AB,
    SKERNEL_98MX610BS   = 0x006011AB,
    SKERNEL_98EX110BS   = 0x006711AB,
    SKERNEL_98EX111BS   = 0x006311AB,
    SKERNEL_98EX112BS   = 0x006511AB,
    SKERNEL_98EX110B    = 0x007711AB,
    SKERNEL_98EX120B    = 0x005711AB,
    SKERNEL_98EX120B_   = 0x005611AB,
    SKERNEL_98EX111B    = 0x007311AB,
    SKERNEL_98EX112B    = 0x007511AB,
    SKERNEL_98EX121B    = 0x005311AB,
    SKERNEL_98EX122B    = 0x005511AB,
    SKERNEL_98EX128B    = 0x005011AB,
    SKERNEL_98EX129B    = 0x005211AB,

    /* Cheetah */
    SKERNEL_98DX270     = 0xD80411AB,
    SKERNEL_98DX260     = 0xD80C11AB,
    SKERNEL_98DX250     = 0xD81411AB,
    SKERNEL_98DX803     = 0xD80611AB,
    SKERNEL_98DX249     = 0xD82611AB,
    SKERNEL_98DX269     = 0xD82411AB,
    SKERNEL_98DX169     = 0xD82711AB,

    SKERNEL_98DX248     = 0xD85411AB,
    SKERNEL_98DX262     = 0xD84E11AB,
    SKERNEL_98DX268     = 0xD84C11AB,

    SKERNEL_98DX163     = 0xD2D511AB,
    SKERNEL_98DX166     = 0xD81511AB,
    SKERNEL_98DX167     = 0xD81111AB,

    SKERNEL_98DX243     = 0xD2D411AB,
    SKERNEL_98DX253     = 0xD81011AB,/*same as 98dx247*/
    SKERNEL_98DX263     = 0xD80811AB,
    SKERNEL_98DX273     = 0xD80011AB,
    SKERNEL_98DX247     = SKERNEL_98DX253,
    SKERNEL_98DX107B0   = 0xD3D311AB, /* DX106 B0 and DX107 B0*/
    SKERNEL_98DX106     = SKERNEL_98DX107B0,
    SKERNEL_98DX107     = 0xD81311AB, /* DX106 A2 and DX107 A2 */
    SKERNEL_98DX133     = 0xD80D11AB,

    /* Tiger */
    SKERNEL_98EX116     = 0x012111AB,
    SKERNEL_98EX106     = 0x012A11AB,
    SKERNEL_98EX108     = 0x012B11AB,
    SKERNEL_98EX126     = 0x011111AB,
    SKERNEL_98EX136     = 0x015111AB,

    SKERNEL_98EX126V0   = 0x011311AB,

    /* Tiger Drop In devices */
    SKERNEL_98EX116DI   = 0x012511AB,
    SKERNEL_98EX126DI   = 0x011511AB,
    SKERNEL_98EX136DI   = 0x015511AB,

    /* Fabric Adapter */
    SKERNEL_98FX900     = 0x00000900,
    SKERNEL_98FX910     = 0x00000910,
    SKERNEL_98FX915     = 0x00000915,
    SKERNEL_98FX920     = 0x00000920,
    SKERNEL_98FX930     = 0x0000093A,
    SKERNEL_98FX950     = 0x000FA20E,

    /* Cross bar */
    SKERNEL_98FX9110    = 0x00009110,
    SKERNEL_98FX9210    = 0x00009210,
    SKERNEL_98FX9310    = 0x00009310,

    /* Cheetah2 */
    SKERNEL_98DX255     = 0xD91411AB,
    SKERNEL_98DX265     = 0xD90C11AB,
    SKERNEL_98DX275     = 0xD90411AB,
    SKERNEL_98DX285     = 0xD91C11AB,
    SKERNEL_98DX804     = 0xD91811AB,
    SKERNEL_98DX125     = 0xD91511AB,
    SKERNEL_98DX145     = 0xD90D11AB,

    /* Cheetah3 */
/*    SKERNEL_98DX286     = 0xDB0011AB,*/
    SKERNEL_98DX806     = 0xDB0F11AB,
    SKERNEL_98DX5128    = 0xDB0011AB,
    SKERNEL_98DX5128_1  = 0xDB1011AB,
    SKERNEL_98DX5124    = 0xDB4011AB,
    SKERNEL_98DX5126    = 0xDB6011AB,
    SKERNEL_98DX5127    = 0xDB7011AB,
    SKERNEL_98DX5129    = 0xDB9011AB,
    SKERNEL_98DX5151    = 0xDB1211AB,
    SKERNEL_98DX5152    = 0xDB2211AB,
    SKERNEL_98DX5154    = 0xDB4211AB,
    SKERNEL_98DX5155    = 0xDB5211AB,
    SKERNEL_98DX5156    = 0xDB6211AB,
    SKERNEL_98DX5157    = 0xDB7211AB,
    /* ch3 XG */
    SKERNEL_98DX8110    = 0xDB0111AB,
    SKERNEL_98DX8108    = 0xDB8111AB,
    SKERNEL_98DX8109    = 0xDB9111AB,
    SKERNEL_98DX8110_1  = 0xDB1111AB,

    /* Cheetah3+ */
    SKERNEL_98DX287     = 0xDB0F11AB,/* dummy number (for now) */

    /* Puma */
    SKERNEL_98EX240     = 0xC24011AB,
    SKERNEL_98EX240_1   = 0xC20011AB,
    SKERNEL_98EX241     = 0xC24111AB,
    SKERNEL_98MX840     = 0x222211AB, /* dummy number */

    SKERNEL_98EX2106    = 0xC30011AB,
    SKERNEL_98EX2206    = 0xC34011AB,
    SKERNEL_98MX2306    = 0xC36011AB,
    SKERNEL_98EX8261    = 0xC34211AB,
    SKERNEL_98EX8301    = 0xC34411AB,
    SKERNEL_98EX8303    = 0xC30411AB,
    SKERNEL_98EX8501    = 0xC30611AB,

/* Puma devices : 2 XG devices */
    SKERNEL_98EX260     = 0xC24111AB,
    SKERNEL_98MX860     = 0x444411AB, /* dummy number */

/* Puma2 devices : 2 port XG devices */
    SKERNEL_98EX2110    = 0xC30111AB,
    SKERNEL_98EX2210    = 0xC34111AB,
    SKERNEL_98MX2310    = 0xC36111AB,
    SKERNEL_98EX8262    = 0xC34311AB,
    SKERNEL_98EX8302    = 0xC34511AB,
    SKERNEL_98EX8305    = 0xC30511AB,
    SKERNEL_98EX8502    = 0xC30711AB,


    /********/
    /* xCat */
    /********/
                                                    /*  FE       GE       Stack  Stack     Stack     Stack          */
                                                    /*                    GE     GE/2.5GE  GE/2.5/   GE/2.5/        */
                                                    /*                           (DQX)     10/12GE   5/10/12GE      */
                                                    /*                                     (DQX/DHX) (DQX/DHX/XAUI) */

    /*98DX1100 Series: TomCat-FE (SMB L2+) - 600MHz, 16KB/128KB Cache  */
    SKERNEL_98DX1101       = 0xDC5111AB,            /*  8         0        0        2        0        0 */
    SKERNEL_98DX1111       = 0xDC6111AB,            /*  16        0        0        2        0        0 */
    SKERNEL_98DX1122       = 0xDCB111AB,            /*  24        0        0        4        0        0 */
    SKERNEL_98DX1123       = 0xDCB311AB,            /*  24        0        0        4        0        0 */
    SKERNEL_98DX1142       = 0xDCB911AB,            /*  48        0        0        4        0        0 */

    /*98DX2100 Series: BobCat-FE (L3+ / Metro) - 800MHz, 16KB/256K Cache*/
    SKERNEL_98DX2101       = 0xDD5111AB,            /*  8         0        0        2        0        0 */
    SKERNEL_98DX2112       = 0xDDA111AB,            /*  16        0        0        4        0        0 */
    SKERNEL_98DX2122       = 0xDDB111AB,            /*  24        0        0        4        0        0 */
    SKERNEL_98DX2123       = 0xDDB311AB,            /*  24        0        0        4        0        0 */
    SKERNEL_98DX2142       = 0xDDB911AB,            /*  48        0        0        4        0        0 */
    SKERNEL_98DX2151       = 0xDD7511AB,            /*  24        0        0        2        0        2 */
    SKERNEL_98DX2161       = 0xDD7D11AB,            /*  48        0        0        2        0        2 */

    /*98DX3000 Series: AlleyCat-GE (Entry L2) -  333MHz, 0K Cache, 128KB memory*/
    SKERNEL_98DX3001       = 0xDE5011AB,            /*  0         8        2        0        0        0 */
    SKERNEL_98DX3010       = 0xDE2011AB,            /*  0        16        0        0        0        0 */
    SKERNEL_98DX3011       = 0xDE6011AB,            /*  0        16        2        0        0        0 */
    SKERNEL_98DX3020       = 0xDE3011AB,            /*  0        24        0        0        0        0 */
    SKERNEL_98DX3021       = 0xDE7011AB,            /*  0        24        2        0        0        0 */
    SKERNEL_98DX3022       = 0xDEB011AB,            /*  0        24        4        0        0        0 */

    /*98DX3100 Series: TomCat-GE (SMB L2+) - 600MHz, 16KB/128KB Cache*/
    SKERNEL_98DX3101       = 0xDC5011AB,            /*  0         8        2        0        0        0 */
    SKERNEL_98DX3110       = 0xDC2011AB,            /*  0        16        0        0        0        0 */
    SKERNEL_98DX3111       = 0xDCE011AB,            /*  0        16        0        2        2        0 */
    SKERNEL_98DX3120       = 0xDC3011AB,            /*  0        24        0        0        0        0 */
    SKERNEL_98DX3121       = 0xDCF011AB,            /*  0        24        0        2        2        0 */
    SKERNEL_98DX3122       = 0xDCB411AB,            /*  0        24        0        0        0        4 */
    SKERNEL_98DX3123       = 0xDCB611AB,            /*  0        24        0        0        0        4 */
    SKERNEL_98DX3124       = 0xDC7411AB,            /*  0        24        0        0        2        2 */
    SKERNEL_98DX3125       = 0xDC7611AB,            /*  0        24        0        0        2        2 */
    SKERNEL_98DX3141       = 0xDCF811AB,            /*  0        48        0        0        2        0 */
    SKERNEL_98DX3142       = 0xDCBC11AB,            /*  0        48        0        0        0        4 */

    /*98DX4100 Series: BobCat-GE (L3+ / Metro) - 800MHz,16KB/256KB Cache*/
    SKERNEL_98DX4101       = 0xDD5011AB,            /*  0         8        0        2        0        0 */
    SKERNEL_98DX4102       = 0xDD9011AB,            /*  0         8        0        4        0        0 */
    SKERNEL_98DX4103       = 0xDD8011AB,            /*  0         6        0        4        0        0 */
    SKERNEL_98DX4110       = 0xDDA011AB,            /*  0        16        0        4        0        0 */
    SKERNEL_98DX4120       = 0xDDB011AB,            /*  0        24        0        4        0        0 */
    SKERNEL_98DX4121       = 0xDD7411AB,            /*  0        24        0        0        0        2 */
    SKERNEL_98DX4122       = 0xDDB411AB,            /*  0        24        0        0        0        4 */
    SKERNEL_98DX4123       = 0xDDB611AB,            /*  0        24        0        0        0        4 */
    SKERNEL_98DX4140       = 0xDDB811AB,            /*  0        48        0        4        0        0 */
    SKERNEL_98DX4141       = 0xDD7C11AB,            /*  0        48        0        0        0        2 */
    SKERNEL_98DX4142       = 0xDDBC11AB,            /*  0        48        0        0        0        4 */

    SKERNEL_DEVICE_TYPE_LAST
} SKERNEL_DEVICE_TYPE;

typedef enum
{
    SKERNEL_NOT_INITIALIZED_FAMILY,
    SKERNEL_SALSA_FAMILY,
    SKERNEL_NIC_FAMILY,
    SKERNEL_COM_MODULE_FAMILY,
    SKERNEL_TWIST_D_FAMILY,
    SKERNEL_TWIST_C_FAMILY,
    SKERNEL_SAMBA_FAMILY,
    SKERNEL_SOHO_FAMILY,
    SKERNEL_CHEETAH_1_FAMILY,
    SKERNEL_CHEETAH_2_FAMILY,
    SKERNEL_CHEETAH_3_FAMILY,
    SKERNEL_LION_PORT_GROUP_SHARED_FAMILY, /* the device that hold multi-port groups */
    SKERNEL_LION_PORT_GROUP_FAMILY,  /* the device that is a single port group, one of 4 in the Lion*/
    SKERNEL_XCAT_FAMILY,
    SKERNEL_XCAT2_FAMILY,
    SKERNEL_TIGER_FAMILY,
    SKERNEL_FA_FOX_FAMILY,
    SKERNEL_FAP_DUNE_FAMILY,
    SKERNEL_XBAR_CAPOEIRA_FAMILY,
    SKERNEL_FE_DUNE_FAMILY,
    SKERNEL_PUMA_FAMILY,
    SKERNEL_EMBEDDED_CPU_FAMILY,

    SKERNEL_PHY_SHELL_FAMILY, /* the PHY (QUAD PHY) that hold multi-port groups */
    SKERNEL_PHY_CORE_FAMILY,   /* the PHY that is a single channel, one of 4 in QUAD_PHY */

    SKERNEL_MACSEC_FAMILY,    /* the MACSEC family */

    SKERNEL_LION2_PORT_GROUP_SHARED_FAMILY, /* the device that hold multi-port groups */
    SKERNEL_LION2_PORT_GROUP_FAMILY,  /* the device that is a single port group, one of 8 in the Lion2 */

    SKERNEL_LION3_PORT_GROUP_SHARED_FAMILY, /* the device that hold multi-port groups */
    SKERNEL_LION3_PORT_GROUP_FAMILY,  /* the device that is a single port group, one of 8 in the Lion3 */

    SKERNEL_PUMA3_SHARED_FAMILY, /* the device that hold multi-port groups */
    SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY,  /* the device that is a single network+fabric cores, one of 2 in the Puma3 */

    SKERNEL_BOBCAT2_FAMILY,
    SKERNEL_BOBK_CAELUM_FAMILY,
    SKERNEL_BOBK_CETUS_FAMILY,

    SKERNEL_BOBCAT3_FAMILY,

    SKERNEL_XCAT3_FAMILY,

    SKERNEL_EMPTY_FAMILY,  /* empty -- device not exists */
    SKERNEL_LAST_FAMILY
}SKERNEL_DEVICE_FAMILY_TYPE;


#define SKERNEL_DEVICE_FAMILY_NIC(deviceType)\
    (deviceType == SKERNEL_NIC ? 1 : 0 )

#define SKERNEL_DEVICE_FAMILY_COM_MODULE(deviceType)   \
    (deviceType == SKERNEL_COM_MODULE ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_EMBEDDED_CPU(deviceType)\
    (deviceType == SKERNEL_EMBEDDED_CPU ? 1 : 0 )


#define SKERNEL_DEVICE_FAMILY_TWISTC(deviceType)\
    (deviceType == SKERNEL_98MX610B ? 1 :      \
     deviceType == SKERNEL_98MX620B ? 1 :      \
     deviceType == SKERNEL_98EX110B ? 1 :      \
     deviceType == SKERNEL_98EX120B ? 1 :      \
     deviceType == SKERNEL_98EX111B ? 1 :      \
     deviceType == SKERNEL_98EX112B ? 1 :      \
     deviceType == SKERNEL_98EX121B ? 1 :      \
     deviceType == SKERNEL_98EX122B ? 1 :      \
     deviceType == SKERNEL_98EX128B ? 1 :      \
     deviceType == SKERNEL_98EX129B ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_SAMBA(deviceType)\
    (deviceType == SKERNEL_98MX615D ? 1 :      \
     deviceType == SKERNEL_98MX625D ? 1 :      \
     deviceType == SKERNEL_98MX635D ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_SALSA_A1(deviceType)\
    (deviceType == SKERNEL_98DX160 ? 1 :       \
     deviceType == SKERNEL_98DX161 ? 1 :       \
     deviceType == SKERNEL_98DX240 ? 1 :       \
     deviceType == SKERNEL_98DX241 ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_SALSA_A2(deviceType)\
    (deviceType == SKERNEL_98DX1602 ? 1 :      \
     deviceType == SKERNEL_98DX1612 ? 1 :      \
     deviceType == SKERNEL_98DX2402 ? 1 :      \
     deviceType == SKERNEL_98DX2412 ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_SOHO(deviceType)  \
    (deviceType == SKERNEL_SAPPHIRE ? 1 :       \
     deviceType == SKERNEL_RUBY     ? 1 :       \
     SKERNEL_DEVICE_FAMILY_SOHO2(deviceType))

#define SKERNEL_DEVICE_FAMILY_SOHO2(deviceType)  \
    (deviceType == SKERNEL_OPAL     ? 1 :        \
     deviceType == SKERNEL_JADE     ? 1 :        \
     SKERNEL_DEVICE_FAMILY_SOHO_PLUS(deviceType))

#define SKERNEL_DEVICE_FAMILY_SOHO_PLUS(deviceType)  \
    (deviceType == SKERNEL_OPAL_PLUS ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_FA(deviceType)    \
    (deviceType == SKERNEL_98FX910 ? 1 :        \
     deviceType == SKERNEL_98FX900 ? 1 :        \
     deviceType == SKERNEL_98FX920 ? 1 :        \
     deviceType == SKERNEL_98FX915 ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_XBAR(deviceType)    \
    (deviceType == SKERNEL_98FX9210 ? 1 :        \
     deviceType == SKERNEL_98FX9110 ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_SALSA(deviceType)\
    (SKERNEL_DEVICE_FAMILY_SALSA_A1(deviceType)\
     || SKERNEL_DEVICE_FAMILY_SALSA_A2(deviceType))

#define SKERNEL_DEVICE_FAMILY_TWISTD(deviceType)\
    (deviceType == SKERNEL_98EX100D ? 1 :       \
     deviceType == SKERNEL_98EX110D ? 1 :       \
     deviceType == SKERNEL_98EX115D ? 1 :       \
     deviceType == SKERNEL_98EX120D ? 1 :       \
     deviceType == SKERNEL_98EX125D ? 1 :       \
     deviceType == SKERNEL_98EX130D ? 1 :       \
     deviceType == SKERNEL_98EX135D ? 1 : 0)

#define SKERNEL_IS_CHEETAH2_ONLY_DEV(dev) \
    skernelIsCheetah2OnlyDev(dev)

#define SKERNEL_IS_CHEETAH3_ONLY_DEV(dev) \
    skernelIsCheetah3OnlyDev(dev)

/* ONLY cheetah 3+ (3 plus)*/
#define SKERNEL_IS_CHEETAH3P_ONLY_DEV(dev) \
    (dev->deviceType == SKERNEL_98DX287 ? 1 :    \
     dev->deviceType == SKERNEL_DXCH3P  ? 1 :    \
     0)

/* xCat devices and above */
#define SKERNEL_IS_XCAT_DEV(dev)   \
    skernelIsXcatDev(dev)

/* xCat2 devices and above */
#define SKERNEL_IS_XCAT2_DEV(dev)   \
    skernelIsXcat2Dev(dev)

/* xCat Revision A1 devices and above */
#define SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)     \
    (skernelIsXcatDev(dev) && (!((dev)->isXcatA0Features)))

/* is device is 'ch3 and above' (also ch3p,xCat,Lion,Lion2,Lion3) */
#define SKERNEL_IS_CHEETAH3_DEV(dev) \
    skernelIsCheetah3Dev(dev)

/* is cheetah 1 only device (not ch2,3..) */
#define SKERNEL_IS_CHEETAH1_ONLY_DEV(dev) \
    skernelIsCheetah1OnlyDev(dev)

#define SKERNEL_IS_DEVICE_CHEETAH_B0(dev)\
    (dev->deviceType == SKERNEL_DXCH_B0   ? 1 :     \
     dev->deviceType == SKERNEL_98DX107B0 ? 1 :     \
     0)

#define SKERNEL_DEVICE_FAMILY_CHEETAH_1_ONLY(dev)\
    skernelIsCheetah1OnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_CHEETAH_2_ONLY(dev)\
    skernelIsCheetah2OnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev)\
    skernelIsCheetah3OnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_CHEETAH(dev)\
    skernelIsCheetahDev(dev)

#define SKERNEL_DEVICE_FAMILY_XCAT_ONLY(dev)\
    skernelIsXcatOnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_XCAT2_ONLY(dev)\
    skernelIsXcat2OnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_XCAT3_ONLY(dev)\
    skernelIsXcat3OnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_LION_PORT_GROUP_ONLY(dev)\
    skernelIsLionPortGroupOnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_LION_PORT_GROUP_DEV(dev)\
    skernelIsLionPortGroupDev(dev)

/* Lion Revision B0 devices and above */
#define SKERNEL_IS_LION_REVISON_B0_DEV(dev)     \
    (skernelIsLionPortGroupDev(dev) && (!((dev)->isXcatA0Features)))

#define SKERNEL_DEVICE_FAMILY_LION2_PORT_GROUP_ONLY(dev)\
    skernelIsLion2PortGroupOnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_LION2_SHELL_ONLY(dev)\
    skernelIsLion2ShellOnlyDev(dev)

#define SKERNEL_DEVICE_FAMILY_LION2_PORT_GROUP_DEV(dev)\
    skernelIsLion2PortGroupDev(dev)

/* Lion2 devices and above */
#define SKERNEL_IS_LION2_DEV(dev)     \
    (skernelIsLion2PortGroupDev(dev))

#define SKERNEL_DEVICE_FAMILY_TIGER(deviceType)\
    (deviceType == SKERNEL_98EX116 ? 1 :       \
     deviceType == SKERNEL_98EX126 ? 1 :       \
     deviceType == SKERNEL_98EX136 ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_FAP_DUNE(deviceType)    \
    (deviceType == SKERNEL_98FX930 ? 1 :            \
     deviceType == SKERNEL_98FX950 ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_FE_DUNE(deviceType)    \
    (deviceType == SKERNEL_98FX9310 ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY_PUMA(deviceType)      \
    (deviceType == SKERNEL_98EX240 ? 1 :            \
     deviceType == SKERNEL_98EX241 ? 1 : 0)

#define SKERNEL_DEVICE_FAMILY(dev) \
    (SKERNEL_DEVICE_FAMILY_NIC(dev->deviceType)      ? SKERNEL_NIC_FAMILY     :  \
     SKERNEL_DEVICE_FAMILY_COM_MODULE(dev->deviceType) ? SKERNEL_COM_MODULE_FAMILY : \
     SKERNEL_DEVICE_FAMILY_SOHO(dev->deviceType)     ? SKERNEL_SOHO_FAMILY    :  \
     SKERNEL_DEVICE_FAMILY_FA(dev->deviceType)       ? SKERNEL_FA_FOX_FAMILY  :  \
     SKERNEL_DEVICE_FAMILY_XBAR(dev->deviceType)     ? SKERNEL_XBAR_CAPOEIRA_FAMILY : \
     SKERNEL_DEVICE_FAMILY_FAP_DUNE(dev->deviceType) ? SKERNEL_FAP_DUNE_FAMILY : \
     SKERNEL_DEVICE_FAMILY_FE_DUNE(dev->deviceType)  ? SKERNEL_FE_DUNE_FAMILY :  \
     SKERNEL_DEVICE_FAMILY_EMBEDDED_CPU(dev->deviceType)      ? SKERNEL_EMBEDDED_CPU_FAMILY  :  \
     SKERNEL_NOT_INITIALIZED_FAMILY)


#define SKERNEL_FABRIC_ADAPTER_DEVICE_FAMILY(deviceType) \
    (SKERNEL_DEVICE_FAMILY_FA(deviceType)       ? 1 :  \
     SKERNEL_DEVICE_FAMILY_XBAR(deviceType)     ? 1 :  \
     SKERNEL_DEVICE_FAMILY_FAP_DUNE(deviceType) ? 1 : \
     SKERNEL_DEVICE_FAMILY_FE_DUNE(deviceType)  ? 1 : 0)


/* frame type */
typedef enum
{
    SKERNEL_FRAME_TYPE_OTHER_E = 0,
    SKERNEL_FRAME_TYPE_ARP_E,
    SKERNEL_FRAME_TYPE_IGMP_E,
    SKERNEL_FRAME_TYPE_BPDU_E,
    SKERNEL_FRAME_TYPE_MLD_E
}SKERNEL_FRAME_TYPE;

/* source type of the buffer */
typedef enum
{
    SMAIN_SRC_TYPE_SLAN_E  = 1,
    SMAIN_SRC_TYPE_CPU_E,
    SMAIN_SRC_TYPE_UPLINK_E,
    SMAIN_SRC_TYPE_LOOPBACK_PORT_E, /* the packet cam from port that is in MAC/PCS loopback mode (not from SLAN) */
    SMAIN_SRC_TYPE_INTERNAL_CONNECTION_E, /* the packet came from port that is internal connection (not from SLAN) */

    SMAIN_SRC_TYPE___LAST__E = 0x0000FFFF
} SMAIN_SRC_TYPE_ENT;

/*
 * Typedef: enum SMAIN_DIRECTION_ENT
 *
 * Description:
 *      This enum defines direction: ingress or egress
 * Fields:
 *     SMAIN_DIRECTION_INGRESS_E - the direction is ingress
 *     SMAIN_DIRECTION_EGRESS_E  - the direction is egress
 *
 *  Comments:
 */
typedef enum{
    SMAIN_DIRECTION_INGRESS_E,
    SMAIN_DIRECTION_EGRESS_E
}SMAIN_DIRECTION_ENT;

/* message type of the buffer */
typedef enum
{
    SMAIN_MSG_TYPE_FRAME_E,
    SMAIN_MSG_TYPE_FDB_UPDATE_E,
    SMAIN_CPU_MAILBOX_MSG_E,
    SMAIN_CPU_PCSPING_MSG_E,
    SMAIN_CPU_FDB_ACT_TRG_E,
    SMAIN_CPU_TX_SDMA_QUEUE_E,
    SMAIN_CPU_FDB_AUTO_AGING_E,
    SMAIN_LINK_CHG_MSG_E,
    SMAIN_REACHABILITY_MSG_E,
    SMAIN_INTERRUPTS_MASK_REG_E,
    SMAIN_MSG_TYPE_FDB_UPLOAD_E,
    SMAIN_MSG_TYPE_CNC_FAST_DUMP_E,
    SMAIN_MSG_TYPE_SOFT_RESET_E,
    SMAIN_MSG_TYPE_GENERIC_FUNCTION_E /* see GENERIC_MSG_FUNC . */

    /* DONT add new values to enum -->
       check if SMAIN_MSG_TYPE_GENERIC_FUNCTION_E can support the new message that you need */

}SMAIN_MSG_TYPE_ENT;

/* vlan_cmd */
typedef enum
{
    SKERNEL_NOT_CHANGE_E = 0,
    SKERNEL_ADD_TAG_E,
    SKERNEL_REMOVE_TAG_E,
    SKERNEL_MODIFY_TAG_E
}SKERNEL_VLAN_CMD_ENT;

/* Traffic commands  */
typedef enum
{
    SKERNEL_FORWARD_E = 0,
    SKERNEL_DROP_E,
    SKERNEL_INTERVENTION_E,
    SKERNEL_TRAP_CPU_E
}SKERNEL_COMMON_CMD_ENT;

/* egress packet type */
typedef enum
{
    SKERNEL_EGRESS_PACKET_IPV4_E,/*0*/
    SKERNEL_EGRESS_PACKET_IPV6_E,/*1*/
    SKERNEL_EGRESS_PACKET_MPLS_E,/*2*/
    SKERNEL_EGRESS_PACKET_OTHER_E/*3*/
}SKERNEL_EGRESS_PACKET_TYPE_ENT;

/* STP state */
typedef enum
{
    SKERNEL_STP_DISABLED_E = 0,
    SKERNEL_STP_BLOCK_LISTEN_E,
    SKERNEL_STP_LEARN_E,
    SKERNEL_STP_FORWARD_E,
}SKERNEL_STP_ENT;

/* L2 movement */
typedef enum {
    SKERNEL_L2_NO_MOVE_E = 0,
    SKERNEL_L2_DYNAMIC_MOVE_E,
    SKERNEL_L2_STATIC_MOVE_E
}SKERNEL_L2MOVE_ENT;

/* Packet commands */
typedef enum {
    SKERNEL_PKT_FORWARD_E = 0,
    SKERNEL_PKT_DROP_E,
    SKERNEL_PKT_TRAP_CPU_E,
    SKERNEL_PKT_MIRROR_CPU_E
}SKERNEL_PACKET_CMD_ENT;

/* MAC address type
    0 - unicast, MAC_DA[40] = 1'b0
    1 - multicast, MAC_DA[40] = 1'b1
    2 - broadcast, MAC_DA = 48'hffffffffffff
    3 - ARP broadcast - packet is ARP (EtherType = 0x806) and
                        MAC_DA = 48'hffffffffffff
*/
typedef enum {
    SKERNEL_UNICAST_MAC_E = 0,
    SKERNEL_MULTICAST_MAC_E,
    SKERNEL_BROADCAST_MAC_E,
    SKERNEL_BROADCAST_ARP_E
} SKERNEL_MAC_TYPE_ENT;

/*
  L2 encapsulation for Ethernet frames
  SKERNEL_ETHERNET_II_E - Length/EtherType > 1500 decimal
                         AND Length/EtherType != 0x8870
                         (EtherType for Jumbo LLC packets)
  SKERNEL_LLC_E         - Length/EtherType <= 1500 OR Length/EtherType = 0x8870
                          (EtherType for Jumbo LLC packets),
                          AND the LLC <DSAP-SSAP-Control> != 0xAA-0xAA-0x03.
  SKERNEL_LLC_SNAP_E    - Length/EtherType <= 1500 OR Length/EtherType = 0x8870
                          (EtherType for Jumbo LLC packets),
                          AND the LLC <DSAP-SSAP-Control> == 0xAA-0xAA-0x03.

*/
typedef enum {
    SKERNEL_ETHERNET_II_E = 1,
    SKERNEL_LLC_E = 2,
    SKERNEL_LLC_SNAP_E = 4
} SKERNEL_L2_ENCAPSULATION_TYPE_ENT;



/* Frame type */
typedef enum {
    SKERNEL_BPDU_E = 4,
    SKERNEL_ARP_E,
    SKERNEL_IGMP_E,
    SKERNEL_UCAST_MPLS_E,
    SKERNEL_MCAST_MPLS_E,
    SKERNEL_FRAME_TYPE_IPV4_E,
    SKERNEL_FRAME_TYPE_IPV6_E,
    SKERNEL_FRAME_TYPE_RIPV1_E
} SKERNEL_FRAME_TYPE_ENT;


/* Flow mirror cause */
typedef enum {
    SKERNEL_NO_MIRROR_E = 0,
    SKERNEL_MIRR_RST_FIN_E,
    SKERNEL_MIRR_FLOW_ENTRY_E,
    SKERNEL_MIRROR_RESERVED_E
} SKERNEL_MIRROR_CAUSE_E;

/* Last Level Treated */
typedef enum {
    SKERNEL_ETHER_BRIDGE_E = 0,
    SKERNEL_IPV4_ROUT_E,
    SKERNEL_IPV6_ROUT_E,
    SKERNEL_MPLS_E,
    SKERNEL_RESERVED_E,
    SKERNEL_L2CE_E
} SKERNEL_LAST_LEVEL_E;

/* Protocols Encapsulation */
typedef enum {
    SKERNEL_ENCAP_ETHER_E = 0,
    SKERNEL_ENCAP_LLC_E,
    SKERNEL_ENCAP_LLC_SNAP_E
} SKERNEL_PROT_ENCAP_E;

typedef enum {
    SKERNEL_TCP_FLOW_E = 0,
    SKERNEL_UDP_FLOW_E,
    SKERNEL_MPLS_FLOW_E,
    SKERNEL_IP_FRAGMENT_E,
    SKERNEL_IP_OTHER_E,
    SKERNEL_ETHERNET_E,
    SKERNEL_OTHER_E,
    SKERNEL_IPV6_E,
} SKERNEL_FLOW_TEMPLATE_E;

/*  Additions for IPv4 support */
typedef  enum {
    IPV4_LPM_ECMP_ROUT_METHOD_E = 0,     /* Pointer to an ECMP entry */
    IPV4_LPM_QOS_ROUT_METHOD_E = 1,      /* Pointer to a QOS route entry */
    IPV4_LPM_REGULAR_ROUT_METHOD_E = 2   /* Pointer to a regular route entry */
}IPV4_LPM_ROUT_METHOD_ENT;

typedef enum{
    IPV4_LPM_REGULAR_BUCKET_E = 0,       /* Next pointer is a pointer to
                                           another Regular Bucket*/
    IPV4_LPM_COMPRESS_1_BUCKET_E = 1,    /* Next pointer is a pointer to
                                           a compressed Bucket type 1*/
    IPV4_LPM_COMPRESS_2_BUCKET_E = 2,   /* Next pointer is a pointer to
                                           a compressed Bucket type 2*/
    IPV4_LPM_NEXT_HOP_BUCKET_E = 3      /* Next pointer is a pointer to
                                           a Next Hop Route Entry. */
}IPV4_LPM_BUCKET_TYPE_ENT;

typedef enum{
    IPV4_LPM_RESULT_NOT_FOUND_E = 0,    /* LPM prefix not found */
    IPV4_LPM_RESULT_FOUND_E = 1         /* LPM prefix found */
}IPV4_LPM_RESULT_ENT;

typedef enum {
    IPV4_CMD_TRANSPARENT_E = 0,         /* Pass transparently */
    IPV4_CMD_ROUT_E,                    /* Route */
    IPV4_CMD_TRAP_E,                    /* Trap to CPU */
    IPV4_CMD_DROP_E                     /* Drop */
}IPV4_ROUT_CMD_ENT;

typedef enum {
    IPV4_LOG_LINK_LAYER_E = 0,          /* LogicalLink Layer */
    IPV4_TUNNEL_E,                      /* tunnel */
    IPV4_MLL_E,                         /* MLL */
}IPV4_OUTLIF_TYPE_ENT;

typedef enum {
    IPV4_ETHER_BRIDGE_E = 0,
    IPV4_ROUT_IPV4_E = 1,
    IPV4_ROUT_IPV6_E = 2,
    IPV4_MPLS_E = 3,
    IPV4_L2CE_E = 5
}IPV4_LLT_ENT;

typedef  enum {
    POLICY_NO_SWITCH_E = 0,
    POLICY_ROUTE_E,
    POLICY_SWITCH_E
}POLICY_SWITCH_TYPE_ENT;

typedef  enum
{
    SKERNEL_UPLINK_TYPE_NONE_E,         /* no uplink exists                  */
    SKERNEL_UPLINK_TYPE_B2B_E,          /* uplink is connected back to back  */
    SKERNEL_UPLINK_TYPE_TO_FA_E   ,     /* fabric adapter                    */
    SKERNEL_UPLINK_TYPE_TO_PP_E         /* packet processor                  */
}SKERNEL_UPLINK_TYPE_ENT;

typedef enum{
    SKERNEL_FDB_UPDATE_NEW_ADDRESS_E,          /* new address message         */
    SKERNEL_FDB_UPDATE_QUERY_ADDRESS_E,        /* query address message       */
    SKERNEL_FDB_UPDATE_QUERY_REPLY_E,          /* query reply message         */
    SKERNEL_FDB_UPDATE_AGED_ADDRESS_E,         /* aged address message        */
    SKERNEL_FDB_UPDATE_TRANSPLANTED_ADDRESS_E, /* transplanted address message*/
    SKERNEL_FDB_UPDATE_SECURITY_ADDRESS_E,     /* security address message    */
    SKERNEL_FDB_UPDATE_QUERY_ADDRESS_INDEX_E,  /* query address index message */
    SKERNEL_FDB_UPDATE_LAST_E
}SKERNEL_FDB_UPDATE_ENT;

typedef enum{
    SKERNEL_EGRESS_COUNTER_EGRESS_BRG_FILTER_E,
    SKERNEL_EGRESS_COUNTER_NOT_FILTER_E,
}SKERNEL_EGRESS_COUNTER_ENT;

/* Description: Flow redirect command options.*/
typedef enum{
    SKERNEL_FLOW_NO_REDIRECT_E,
    SKERNEL_FLOW_MPLS_REDIRECT_E,
    SKERNEL_FLOW_IPV4_REDIRECT_E,
    SKERNEL_FLOW_OUTLIF_REDIRECT_E,
}SKERNEL_FLOW_REDIRECT_ENT;

/* Flow process packet command */
typedef enum{
    SKERNEL_FLOW_DROP_E,
    SKERNEL_FLOW_NO_TRAFFIC_COND_E,
    SKERNEL_FLOW_TRAP_E,
    SKERNEL_FLOW_WITH_TRAFFIC_COND_E
}SKERNEL_FLOW_CMD_ENT;

/* Next layer protocol */
typedef enum {
    NHLFE_NLP_IPV4_E,
    NHLFE_NLP_IPV6_E,
    NHLFE_NLP_MPLS_E,
    NHLFE_NLP_USER_0_E,
    NHLFE_NLP_USER_1_E,
    NHLFE_NLP_RSRV_E,
    NHLFE_NLP_L2CE_ETH_E,
    NHLFE_NLP_L2CE_NON_ETH_E
} NHLFE_NLP_STC;

typedef enum {
    SKERNEL_MPLS_CMD_NONE_E,
    SKERNEL_MPLS_CMD_SWAP_E,
    SKERNEL_MPLS_CMD_PUSH1_LABLE_E,
    SKERNEL_MPLS_CMD_POP1_LABLE_E,
    SKERNEL_MPLS_CMD_POP2_LABLE_E,
    SKERNEL_MPLS_CMD_POP3_LABLE_E,
    SKERNEL_MPLS_CMD_RESERVED_E,
    SKERNEL_MPLS_CMD_POP_SWAP_E
}SKERNEL_MPLS_CMD_TYPE_ENT;

typedef enum {
    TAG_CMD_TO_CPU_E,
    TAG_CMD_FROM_CPU_E,
    TAG_CMD_TO_TARGET_SNIFFER_E,
    TAG_CMD_FORWARD_E
} TAG_COMMAND_E;

/* enum only for EXT DSA tag. for the field of : marvellTaggedExtended */
typedef enum {
    SKERNEL_EXT_DSA_TAG_1_WORDS_E = 0,
    SKERNEL_EXT_DSA_TAG_2_WORDS_E = 1,
    SKERNEL_EXT_DSA_TAG_3_WORDS_E = 2,
    SKERNEL_EXT_DSA_TAG_4_WORDS_E = 3,

    /* last one */
    SKERNEL_EXT_DSA_TAG_LAST_E,
} SKERNEL_EXT_DSA_TAG_TYPE_ENT;

typedef enum {
    MTAG_STANDARD_E =  SKERNEL_EXT_DSA_TAG_1_WORDS_E , /* regular DSA */
    MTAG_EXTENDED_E =  SKERNEL_EXT_DSA_TAG_2_WORDS_E , /* 2 words DSA */
    MTAG_3_WORDS_E  =  SKERNEL_EXT_DSA_TAG_3_WORDS_E , /* 3 words DSA */
    MTAG_4_WORDS_E  =  SKERNEL_EXT_DSA_TAG_4_WORDS_E , /* 4 words DSA */

    MTAG_TYPE_NONE_E  = 0xFF

}DSA_TAG_TYPE_E;

typedef enum {
    SKERNEL_TIGER_INLIF_PER_PORT_ENTRY_E  ,
    SKERNEL_TIGER_INLIF_PER_VLAN_ENTRY_E  ,
    SKERNEL_TIGER_INLIF_PER_VLAN_PORT_HYBRID_ENRTY_E ,
    SKERNEL_TIGER_INLIF_PER_UNKNOWN_E
}SKERNEL_TIGER_INLIF_CALSSIFICATION_TYPE_ENT ;

typedef enum {
    SKERNEL_TIGER_INLIF_PER_VLAN_FULL_E  ,
    SKERNEL_TIGER_INLIF_PER_VLAN_REDUCED_E
}SKERNEL_TIGER_INLIF_PER_VLAN_TYPE_ENT ;

/* Protocol type */
typedef enum {
    SNET_IPV6_HBH_PROT_E = 0,
    SNET_IPV4_ICMP_PROT_E = 1,
    SNET_IGMP_PROT_E = 2,
    SNET_TCP_PROT_E = 6,
    SNET_UDP_PROT_E = 17,
    SNET_IP_GRE_PROT_E = 47,
    SNET_IPV6_ICMP_PROT_E = 58,
    SNET_IPV6_NO_NEXT_HEADER_PROT_E = 59,
    SNET_UDP_LITE_PROT_E = 136,

    SNET_PROT_LAST_E
} PROT_TYPE_ENT;

typedef enum {
    SKERNEL_L3_PROT_TYPE_IPV4_E = 0x0800,
    SKERNEL_L3_PROT_TYPE_IPV6_E = 0x86DD,
    SKERNEL_L3_PROT_TYPE_ARP_E  = 0x0806,

    SKERNEL_L3_PROT_TYPE_LAST   = 0xFFFF
} SKERNEL_L3_PROT_TYPE_ENT;



typedef enum {
    SKERNEL_TIMESTAMP_ACTION_NONE_E = 0,
    SKERNEL_TIMESTAMP_ACTION_FORWARD_E,
    SKERNEL_TIMESTAMP_ACTION_DROP_E,
    SKERNEL_TIMESTAMP_ACTION_CAPTURE_E,
    SKERNEL_TIMESTAMP_ACTION_ADD_TIME_E,
    SKERNEL_TIMESTAMP_ACTION_ADD_CORRECTED_TIME_E,
    SKERNEL_TIMESTAMP_ACTION_CAPTURE_ADD_TIME_E,
    SKERNEL_TIMESTAMP_ACTION_CAPTURE_ADD_CORRECTED_TIME_E,
    SKERNEL_TIMESTAMP_ACTION_ADD_INGRESS_TIME_E,
    SKERNEL_TIMESTAMP_ACTION_CAPTURE_ADD_INGRESS_TIME_E,
    SKERNEL_TIMESTAMP_ACTION_CAPTURE_INGRESS_TIME_E
} SKERNEL_TIMESTAMP_ACTION_ENT;

typedef enum{
    SKERNEL_TIMESTAMP_PACKET_FORMAT_PTPV2_E = 0,
    SKERNEL_TIMESTAMP_PACKET_FORMAT_PTPV1_E,
    SKERNEL_TIMESTAMP_PACKET_FORMAT_Y1731_E,
    SKERNEL_TIMESTAMP_PACKET_FORMAT_NTP_E,
    SKERNEL_TIMESTAMP_PACKET_FORMAT_NTPR_E,
    SKERNEL_TIMESTAMP_PACKET_FORMAT_NTPT_E,
    SKERNEL_TIMESTAMP_PACKET_FORMAT_WAMP_E,
    SKERNEL_TIMESTAMP_PACKET_FORMAT_RESERVED_E
} SKERNEL_TIMESTAMP_PACKET_FORMAT_ENT;

typedef enum{
    SNET_PTP_UDP_CHECKSUM_MODE_CLEAR_E = 0,
    SNET_PTP_UDP_CHECKSUM_MODE_UPDATE_E,
    SNET_PTP_UDP_CHECKSUM_MODE_RESERVED_E,
    SNET_PTP_UDP_CHECKSUM_MODE_NOP_E
} SNET_PTP_UDP_CHECKSUM_MODE_ENT;

/*
 * Typedef: enum PCL_TTI_ACTION_REDIRECT_CMD_ENT
 *
 * Description: enumerator for TTI/PCL redirection target
 *
 * Fields:
 *  PCL_TTI_ACTION_REDIRECT_CMD_NONE_E   - no redirection
 *  PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E - redirection to output interface
 *  PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E - IPNextHop: Redirect to an IP Next Hop Entry. (to LTT index)
 *  PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E -  obsolete in SIP5
 *                          for PCL : PBR
 *                              redirection to PBR interface (policy based routing)
 *                              Use IPCL AE as NHE: regard the Policy Action Entry as a Next Hop Route Entry.
 *                              The Action Entry is regarded a Next Hop Route Entry with a
 *                              format different than for the other cases.
 *                          for TTI:
 *                              for TTI : redirect to VPLS
 *                              NOTE: the simulation not implemented this case ! (for TTI)
 *
 *  PCL_TTI_ACTION_REDIRECT_CMD_VIRT_ROUTER_E - redirect to virtual
 *                        router with the specified Id (VRF ID)
 *  PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E - assign Source logical port. obsolete in SIP5
 *  PCL_TTI_ACTION_REDIRECT_CMD_REPLACE_MAC_SA_E - IPCL sip5 : replace mac SA + Arp pointer (from mac DA)
 */
typedef enum
{
    PCL_TTI_ACTION_REDIRECT_CMD_NONE_E        = 0,
    PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E      = 1,
    PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E  = 2,
    PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E = 3,   /* for PCL : PBR , for TTI : redirect to VPLS */   /*obsolete in SIP5*/
    PCL_TTI_ACTION_REDIRECT_CMD_VIRT_ROUTER_E = 4,
    PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E = 5, /*obsolete in SIP5*/
    PCL_TTI_ACTION_REDIRECT_CMD_REPLACE_MAC_SA_E = 6
} PCL_TTI_ACTION_REDIRECT_CMD_ENT;

/*
 * typedef: struct SNET_TOD_TIMER_STC
 *
 * Description:
 *      TOD timer structure.
 *
 * Fields:
 *      fractionalNanoSecondTimer - TOD fractional nano seconds. 32 bits (SIP5)
 *      nanoSecondTimer           - TOD nano second timer. 30 bits.
 *      secondTimer               - TOD second timer. 64 bits.
 *
 * Comment:
 */
typedef struct
{
    GT_32   fractionalNanoSecondTimer;
    GT_U32  nanoSecondTimer;
    GT_U64  secondTimer;
}SNET_TOD_TIMER_STC;

typedef struct{
    GT_BIT             U;
    GT_BIT             T;
    GT_U32             OffsetProfile;
    GT_BIT             OE;
    SNET_TOD_TIMER_STC timestamp;
}SNET_TIMESTAMP_TAG_INFO_STC;

/*  TTI/PCL
    actions for the redirect action 5 "Assign Logic Port (5)".
*/
typedef struct{
    GT_U32   srcIsTrunk;
    GT_U32   srcTrunkOrPortNum;
    GT_U32   srcDevice;
}SNET_XCAT_TTI_ASSIGN_LOGIC_PORT_ACTION_STC;

/*  descriptor VPLS info */
typedef struct{
    GT_BIT          egressTagStateAssigned; /* flag internal for the simulation to remove the use of value '7' from the egressTagState */
    GT_U32          egressTagState; /*3 bits : set by TTI  .
                            0 = Untagged: Packet is transmitted through this port untagged.
                            1 = Tag0: Tag0 should be used for VLAN tagging of outgoing packets.
                            2 = Tag1: Tag1 should be used for VLAN tagging of outgoing packets.
                            3 = OuterTag0_InnerTag1: The packets should be tagged with two tags, when the outermost is Tag0 and the innermost is Tag1.
                            4 = OuterTag1_InnerTag0: The packets should be tagged with two tags, when the outermost is Tag1 and the innermost is Tag0.
                            5 = PushTag0: Tag0 should be pushed immediately after the MAC SA.
                            6 = PopOuterTag: The outermost VLAN tag of the packet should be removed
                            7 = Not assigned by TTI/PCL. Use VLAN entry value (legacy mode)
                            */
    GT_BIT          unknownSaCmdAssigned; /* flag internal for the simulation to remove the use of value '7' from the unknownSaCmd */
    GT_U32          unknownSaCmd; /*3 bits   : set by PCL,TTI :
                                    Used by Bridge
                                    0 = Forward
                                    1 = MirrorToCPU
                                    2 = TrapToCPU
                                    3 = HardDrop
                                    4 = SoftDrop
                                    5,6 = Reserved
                                    7 = Not assigned by TTI. Using Bridge port configuration register
    */

    GT_U32          srcMeshId; /*2 bits      : set by TTI,PCL */

    GT_U32          pwHasCw; /*1 bit        : set by TTI */
    GT_U32          pwTagMode; /*2 bits     : set by TTI */

    GT_U32          userTaggedAcEnable;/*1 bit : PCL was called serviceVlanKeepMode */

    GT_BIT          targetLogicalPortIsNotVlanMember;/* internal to the simulation .
                            indication inside the EQ , to known if the 'logical port'
                            is considered vlan member or not :
                            1 - not vlan member
                            0 - (maybe) vlan member
                            */
}SKERNEL_VPLS_INFO_STC;

/* defines */
#define DUAL_DEVICE_ID_MASK_CNS                         0xFFFFFFFE

/* macro to check if 2 device ids are equal . NOTE this macro takes into account
    the logic of ignoring lsb when needed */
#define SKERNEL_IS_MATCH_DEVICES_MAC(_dev0,_dev1,_dualDeviceIdEnable) \
        ((((_dev0) == (_dev1)) ||                           \
         (_dualDeviceIdEnable &&                            \
          (((_dev0) & DUAL_DEVICE_ID_MASK_CNS) ==           \
          ((_dev1) & DUAL_DEVICE_ID_MASK_CNS)))) ? 1 : 0)

/* Maximal device supported ports */
#define SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS             256

/* Maximal size of device name name */
#define SKERNEL_DEVICE_NAME_MAX_SIZE_CNS 20

/* Extended data buffer size in words */
#define SKERNEL_EXT_DATA_SIZE_CNS       4

/* Useful macros */
#define MEM_APPEND(memDstPtr, memSrcPtr, bytes)                     \
    memcpy(memDstPtr, memSrcPtr, bytes);                            \
    memDstPtr += bytes

#define SRC_MAC_FROM_DSCR(descrPtr) \
    descrPtr->frameBuf->actualDataPtr + SGT_MAC_ADDR_BYTES

#define DST_MAC_FROM_DSCR(descrPtr) \
    descrPtr->frameBuf->actualDataPtr

#define MEM_APPEND_WITH_LOG(memDstPtr, memSrcPtr, bytes)            \
    /* add LOG info */                                              \
    if(simLogIsOpen())                                              \
    {                                                               \
        __LOG(("MEM_APPEND to [%s] from [%s] num of bytes[%d] \n",  \
            #memDstPtr , #memSrcPtr , bytes));                      \
    }                                                               \
    MEM_APPEND(memDstPtr, memSrcPtr, bytes)

/* MAC Address Table Entry
 *  validEntry  -   Valid 0 - Invalid entry 1 - Valid entry

 *  skipEntry   -   Skip  0 - Don't skip this entry. 1 - Skip this entry

 *  aging       -   1 - Aging bit set (refreshed) by the PP upon receiving a
                        packet from the station corresponding to this entry.
                        When automatically learned, the bit is set to 1.
                    0 - Cleared by the CPU or by PP Aging process.

 *  trunk       -   1 - This MAC address is associated with a trunk and the
                        trunk number is written in the Port# field.
                    0 - The entry is not associated with a trunk.

 *  vid         -   VLAN-ID - If the address was automatically learned,
                    this is the VLAN classification for the learned packet

 *  macAddr     -   The 48 bits of MAC address.

 *  dev         -   Device Number - 7-bit value indicates which one of
                    128 devices in the system is associated with this address

 *  port        -   If this entry is not associated with a Trunk group
                    (<trunk>=0) then this field is the local port number.
                    If this entry is associated with a Trunk group (<trunk>=1)
                    then this field is the Trunk Group Number.

 *  vidx        -   Multicast Group table index. This field holds the direct
                    index to the Multicast Group table.  The VIDx is used in
                    Multicast forwarding.

 *  srcTc       -   Traffic Class associated with a packet with this source MAC,
                    which is used in the Priority Assignment algorithm.

 *  dstTc       -   Traffic Class associated with a packet with this dest. MAC,
                    which is used in the Priority Assignment algorithm.

 *  staticEntry -   0 - The entry can be automatically modified.
                    1 - The entry is static. The Device number and Port number
                        cannot be automatically relearned on a different port.

 *  multiple    -   0 - Forward this packet only to the destination port
                    1 - Forward this packet to all ports in Multicast group
                        (pointed by VIDx).

 *  saCmd       -   SA Command - Controls the action taken when a packet's
                    Source Address matches this entry:
                        0 - Forward
                        1 - Drop (filtering on Source Address)
                        2 - Intervention to CPU (may be dropped by other
                            mechanisms)
                        3 - Control - Unconditionally Trap to CPU

 *  daCmd       -   DA Command - Controls the action taken when a packet's
                    Destination Address matches this entry:
                        0 - Forward
                        1 - Drop (filtering on Source Address)
                        2 - Intervention to CPU (may be dropped by other
                            mechanisms)
                        3 - Control - Unconditionally Trap to CPU

 *  saClass     -   If the packet's Source Address matches this entry,
                    send the packet to the Multi-Field Classifier.

 *  daClass     -   If the packet's Destination Address matches this entry,
                    send the packet to the Multi-Field Classifier.

 *  saCib       -   If the packet's Source Address matches this entry,
                    send the packet to the Customer Interface Bus

 *  daCib       -   If packet's Destination Address matches this entry,
                    send the packet to the Customer Interface Bus.
 */
typedef struct {
    GT_U16                      validEntry;
    GT_U16                      skipEntry;
    GT_U16                      aging;
    GT_U16                      trunk;
    GT_U16                      vid;
    SGT_MAC_ADDR_UNT            macAddr;
    GT_U16                      dev;
    GT_U32                      port;
    GT_U16                      vidx;
    GT_U8                       srcTc;
    GT_U8                       dstTc;
    GT_U8                       staticEntry;
    GT_U8                       multiple;
    SKERNEL_COMMON_CMD_ENT      saCmd;
    SKERNEL_COMMON_CMD_ENT      daCmd;
    GT_BOOL                     saClass;
    GT_BOOL                     daClass;
    GT_BOOL                     saCib;
    GT_BOOL                     daCib;
    GT_BOOL                     daRout;
} SNET_TWIST_MAC_TBL_STC, SNET_TIGER_MAC_TBL_STC;



/*
 * Typedef: struct SKERNEL_UPLINK_STC
 *
 * Description:
 *      Describe an uplink bus in the simulation.
 *
 * Fields:
 *      uplink          : type of uplink.
 *      partnerDeviceID : The id of the device that is connected to this device
 * Comments:
 */
typedef struct
{
    SKERNEL_UPLINK_TYPE_ENT       type;
    GT_U32                        partnerDeviceID;
}SKERNEL_UPLINK_STC;

/*
 * Typedef: struct MPLS_LABELINFO_STC
 *
 * Description:
 *      MPLS label structure
 *
 * Fields:
 *  label   -       label value
 *  exp     -       experimental 3 bits(Class of service)
 *  sbit    -       bottom of stack (1 = last label in entry stack)
 *  ttl     -       time to live
*/
typedef struct{
    GT_U32  label;
    GT_U8   exp;
    GT_U8   sbit;
    GT_U8   ttl;
} MPLS_LABELINFO_STC;

/* SKERNEL_CAPWAP_LOOKUP_SUPPORT_ENT
    description :
        the CAPWAP lookup supported options

    values:
        SKERNEL_CAPWAP_NOT_SUPPORTED_E - device not support CAPWAP
        SKERNEL_CAPWAP_LOOKUP_PCL_TCAM_E - device do lookup inside the PCL tcam
        SKERNEL_CAPWAP_LOOKUP_ROUTER_TCAM_E - device do lookup inside the Router tcam
*/
typedef enum{
    SKERNEL_CAPWAP_NOT_SUPPORTED_E,
    SKERNEL_CAPWAP_LOOKUP_PCL_TCAM_E,
    SKERNEL_CAPWAP_LOOKUP_ROUTER_TCAM_E
}SKERNEL_CAPWAP_LOOKUP_SUPPORT_ENT;

/*
 * Typedef: struct SKERNEL_CAPWAP_REASSEMBLY_KEY_STC
 *
 * Description:
 *      the structure that reassembly lookup key
 *
 * Fields:
 *  isIpv4      - the IP addresses are Ipv4 or Ipv6
 *  srcIp      -  the source IP (part of the lookup key)
 *  dstIp      -  the destination IP (part of the lookup key)
 *  fragmentId -  the fragment Id (part of the lookup key)
*/
typedef struct{
    /* key */
    GT_BIT   isIpv4;
    GT_U32   srcIp[4];
    GT_U32   dstIp[4];
    GT_U16   fragmentId;
}SKERNEL_CAPWAP_REASSEMBLY_KEY_STC;

/*
 * Typedef: struct SKERNEL_CAPWAP_REASSEMBLY_ENTRY_STC
 *
 * Description:
 *      the structure that hold the reassembly entry
 *
 * Fields:
 *  key        -  the key of this entry
 *  isUsed     -  is this entry used or free
 *                support 2 values:
 *                1 - the entry is used
 *                0 - the entry is free
 *  ageValue   -  support 2 values:
 *                1 - the entry is new (set when entry is created / refreshed)
 *                0 - the entry is aged(set after first aging daemon)
 *                NOTE : after second aging daemon the ageValue not changed and
 *                      the 'isUsed' become 'free'
 *  descriptorPtr - pointer to the descriptor of the frame
 *  frameBufferPtr - pointer to the buffer of the frame
 *
 *  totalLength - the length of the payload already received from the fragments.
 *                 NOTE : 1. this value is valid only for fragmentId == 0
 *                        2. every new fragment update this value in entry of fragmentId == 0
 *                        3. the aging will update this value in entry of fragmentId == 0
 *                        4. the entry for fragmentId == 0 must always be 'isUsed = 1'
 *                        5. only the aging of last fragment or the reassembly
 *                           of all fragments , will clear the 'isUsed' bit
 *
*/
typedef struct{
    /* key */
    SKERNEL_CAPWAP_REASSEMBLY_KEY_STC   key;

    /* info */
    GT_BIT   isUsed;
    GT_U32   ageValue;
    void*    descriptorPtr;
    SBUF_BUF_ID frameBufferPtr;

    /* management */
    GT_U32   totalLength;
}SKERNEL_CAPWAP_REASSEMBLY_ENTRY_STC;

typedef struct SKERNEL_DEVICE_OBJECT_T *SKERNEL_DEVICE_OBJECT_T_PTR;
typedef struct SMEM_ACTIVE_MEM_ENTRY_T *SMEM_ACTIVE_MEM_ENTRY_STC_PTR;

/*
 *  Structure : SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC
 *
 *  Description : router tcam parameters --> ch2 and above use it
 *
*/
typedef struct{
    /* number of capacity in ipv4 */
    GT_U32  numEntriesIpv4;
    /* number of capacity in ipv6 (number of ipv4 divided by 4) */
    GT_U32  numEntriesIpv6;
    /* the range of address between one word to another */
    GT_U32  offsetBetweenBulks;
    /* the range of address between one word to another */
    GT_U32  entryWidth;
    /* The range of address between 2 consecutive bank in an entry- number of bytes */
    GT_U32  bankWidth;
}SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC;

struct SKERNEL_DEVICE_TCAM_INFO_STCT;
struct SNET_CHT_POLICY_KEY_STCT;

typedef GT_U32 (SKERNEL_DEVICE_TCAM_LOOK_UP_KEY_WORD_BUILD_FUNC)
(
    IN struct SKERNEL_DEVICE_OBJECT_T *devObjPtr,
    IN struct SKERNEL_DEVICE_TCAM_INFO_STCT *tcamInfoPtr,
    IN struct SNET_CHT_POLICY_KEY_STCT *lookUpKeyPtr,
    IN GT_U32 bankIndex,
    IN GT_U32 wordIndex
);

/*
 *  Structure : SKERNEL_DEVICE_TCAM_INFO_STC
 *
 *  Description : tcam parameters --> general tcam info
 *
*/
typedef struct SKERNEL_DEVICE_TCAM_INFO_STCT{
    /* number of entries */
    GT_U32  numEntries;
    /* the range of address between one word to another */
    GT_U32  offsetBetweenBulks;
    /* the range of address between one word to another */
    GT_U32  entryWidth;
    /* number of banks (that build X) (same number that build Y) */
    GT_U32  numberOfBanks;
    /* number of bits to compare - starting from bit 0 - in each bank */
    GT_U32  numBits;
    /* the start address of x tcam memory */
    GT_U32  xMemoryAddress;
    /* the start address of y tcam memory */
    GT_U32  yMemoryAddress;
    /* call back function to build the current word of the 'key' ,
       according to key data and 'Control bis' */
    SKERNEL_DEVICE_TCAM_LOOK_UP_KEY_WORD_BUILD_FUNC *lookUpKeyWordBuildPtr;
}SKERNEL_DEVICE_TCAM_INFO_STC;


/*
 *  Enumeration : SKERNEL_PORT_STATE_ENT
 *
 *  Description : enum for the state of port, with what set of registers the
 *                device uses of the port
 *                --- this is NOT the 'speed' of the port ---
 *                 NOTEs:
 *              1. in xCat device the some port can be changed from
 *                 working as 'Gig ports' , to work as 'XG ports' on
 *                 runtime !!! (24..27)
 *              2. in xCat in 48+4 device : the ports 0..47 can be FE/GE ports
 *              3. some of cheetah3 devices a port can be changed from
 *                 working as 'Gig ports' , to work as 'XG ports' (0,4,10,12,16,22)
 *                 --> depending on the deviceType (XG device / GE device)
 *              4. in xCat A0 'Gig ports'(24-27) share XG MAC control memory with base address 0x08800000
 *                 in xCat A1 'Gig ports'(24-27) have dedicated memory in own memory range with base address 0x0A800000
 *              5. about the 'XLG port' - the port in Lion B that can be in the
 *                 40G speed is port 10 but the actual memory space is as if the
 *                 port is 12 --> see also SMEM_LION_XLG_PORT_NUM_CNS
 *
*/
typedef enum{
    SKERNEL_PORT_STATE_NOT_EXISTS_E, /* port not exists */

    SKERNEL_PORT_STATE_FE_E,         /* port work with FE registers */
    SKERNEL_PORT_STATE_GE_E,         /* port work with GE registers */
    SKERNEL_PORT_STATE_GE_STACK_A0_E,   /* port work with GE registers of stack ports for XCat A0 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,   /* port work with GE registers of stack ports for XCat A1 */
    SKERNEL_PORT_STATE_XG_E,          /* port work with XG registers */
    SKERNEL_PORT_STATE_XLG_40G_E     /* port work with XLG (40G) registers */
}SKERNEL_PORT_STATE_ENT;

typedef enum{
    SKERNEL_PORT_NATIVE_LINK_DOWN_E,
    SKERNEL_PORT_NATIVE_LINK_UP_E,
}SKERNEL_PORT_NATIVE_LINK_ENT;

/*
 * typedef: enum SKERNEL_PORT_SPEED_ENT
 *
 * Description: Enumeration of port speeds
 *
 * Enumerations:
 *    SKERNEL_PORT_SPEED_10_E    - 10 Mbps
 *    SKERNEL_PORT_SPEED_100_E   - 100 Mbps
 *    SKERNEL_PORT_SPEED_1000_E  - 1 Gbps
 *    SKERNEL_PORT_SPEED_10000_E - 10 Gbps
 *    SKERNEL_PORT_SPEED_NA_E    - for given port parameter speed isn't applicable
 */
typedef enum
{
    SKERNEL_PORT_SPEED_10_E,
    SKERNEL_PORT_SPEED_100_E,
    SKERNEL_PORT_SPEED_1000_E,
    SKERNEL_PORT_SPEED_10000_E,
    SKERNEL_PORT_SPEED_NA_E

}SKERNEL_PORT_SPEED_ENT;

/*
 *  Structure : SKERNEL_INTERNAL_CONNECTION_INFO_STC
 *
 *  Description : info the 'other side' of this port (peer info)
 *      usePeerInfo - is the rest of the info valid ?
 *      peerDevNum - the deviceId of the peer
 *      peerPortNum - the portNum of the peer
 *
 *  NOTE: the 'internal connection' allow to limit the usage of SLANs tasks
*/
typedef struct{
    GT_BOOL         usePeerInfo;
    GT_U8           peerDeviceId;
    GT_U8           peerPortNum;
}SKERNEL_INTERNAL_CONNECTION_INFO_STC;


/*
 * typedef: enum SKERNEL_PORT_LOOPBACK_FORCE_MODE_ENT
 *
 * Description: Enumeration for port loopback forced modes
 *
 * Enumerations:
 *    SKERNEL_PORT_LOOPBACK_NOT_FORCED_E - the port is not force to loopback mode.
 *                   meaning that the behavior depends on the 'loopback enable' in the MAC registers.
 *    SKERNEL_PORT_LOOPBACK_FORCE_ENABLE_E - the port is forced to loopback mode - enable.
 *                          meaning that the behavior is 'loopback enabled'
 *                          regardless to the 'loopback enable' in the MAC registers.
 *                          this mode is needed to support GM devices / other devices
 *                          that can't implement the loopback behavior.
 *
 */
typedef enum{
    SKERNEL_PORT_LOOPBACK_NOT_FORCED_E,
    SKERNEL_PORT_LOOPBACK_FORCE_ENABLE_E,
}SKERNEL_PORT_LOOPBACK_FORCE_MODE_ENT;

/*
 *  Structure : SKERNEL_TIMESTAMP_QUEUE_STC
 *
 *  Description : timestamp queue data base for FIFO implementation
 *      theQueue        - array containing the queue data
 *      actualSize      - the real size of the queue
 *      insertIndex     - line index for next entry insertion
 *      removeIndex     - line index for next entry removal
 *      isFull          - indication that the queue is full
 *      isEmpty         - indication that the queue is empty
 *      overwriteEnable - if queue is full enable writing of new entry on the oldest one.
*/
typedef struct{
    GT_U32 theQueue[MAX_TIMESTAMP_QUEUE_LENGTH_CNS][TIMESTAMP_QUEUE_WIDTH_CNS];
    GT_U32 actualSize;
    GT_U32 insertIndex;
    GT_U32 removeIndex;
    GT_BOOL isFull;
    GT_BOOL isEmpty;
    GT_BOOL overwriteEnable;
} SKERNEL_TIMESTAMP_QUEUE_STC;

/*
 *  Structure : SKERNEL_PORT_INFO_STC
 *
 *  Description : info about the port
 *      state             - state of port
 *      supportMultiState - port can be changed in runTime between states
 *      linkStateWhenNoForce - the link state that the port should be in if
 *                             there is no 'force link down' and no force link up'
 *                             SKERNEL_PORT_NATIVE_LINK_UP_E - link up
 *                             SKERNEL_PORT_NATIVE_LINK_DOWN_E - link down
 *      peerInfo - info about the peer of this port
 *      portCurrentTimeWindow - array of ports current time window;
 *                              Calculated for every incoming packet
 *                              (TS of packet / port window size)
 *
 *      loopbackForceMode - loopback force mode .
 *      isMibCountersCaptured - is MIB counters captured to internal memory
 *      onHoldPacketsArr  - array of on hold packets (for each traffic class --> see descrPtr->queue_priority)
 *      timestampEgressQueue - timestamp queues for capture action
 *      physicalToVirtualPortMappingEn - indication that there is hard-coded physical to virtual port number mapping
 *      virtualMapping - the virtual port number to map to.
 *                       (relevant only if physicalToVirtualPortMappingEn == GT_TRUE).
*/
typedef struct{
    SKERNEL_PORT_STATE_ENT                  state;
    GT_BOOL                                 supportMultiState;
    SKERNEL_PORT_NATIVE_LINK_ENT            linkStateWhenNoForce;
    SKERNEL_INTERNAL_CONNECTION_INFO_STC    peerInfo;
    GT_U32                                  portCurrentTimeWindow;
    SKERNEL_PORT_LOOPBACK_FORCE_MODE_ENT    loopbackForceMode;
    GT_BOOL                                 isMibCountersCaptured;
    SIM_TRANSMIT_QUEUE_STC                  onHoldPacketsArr[NUM_OF_TRAFFIC_CLASSES];
    SKERNEL_TIMESTAMP_QUEUE_STC             timestampEgressQueue[TIMESTAMP_QUEUE_NUM_CNS];
    GT_BOOL                                 physicalToVirtualPortMappingEn;
    GT_U32                                  virtualMapping;
}SKERNEL_PORT_INFO_STC;

/*******************************************************************************
*  SNET_DEV_FRAME_PROC_FUN
*
* DESCRIPTION:
*      Definition of process network frames function
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/

typedef void (* SNET_DEV_FRAME_PROC_FUN ) (
                IN struct SKERNEL_DEVICE_OBJECT_T * devObjPtr,
                IN SBUF_BUF_ID bufferId,
                IN GT_U32 srcPort
);

/*******************************************************************************
*   SNET_CNC_DEV_FAST_UPLOAD_FUN
*
* DESCRIPTION:
*       Process upload CNC block demanded by CPU
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       cncTrigPtr  - pointer to CNC Fast Dump Trigger Register
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef void ( * SNET_CNC_DEV_FAST_UPLOAD_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T * deviceObjPtr,
    IN GT_U32 * cncTrigPtr
);

/*******************************************************************************
*   SNET_DEV_SOFT_RESET_FUN
*
* DESCRIPTION:
*       Process device soft reset
*
* INPUTS:
*       deviceObj           - pointer to device object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef struct SKERNEL_DEVICE_OBJECT_T *  ( * SNET_DEV_SOFT_RESET_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T * deviceObjPtr
);

/*******************************************************************************
*   SFDB_DEV_MSG_PROC_FUN

*
* DESCRIPTION:
*       Process FDB update message.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       fdbMsgPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef void ( * SFDB_DEV_MSG_PROC_FUN ) (
    IN struct SKERNEL_DEVICE_OBJECT_T * devObjPtr,
    IN GT_U8 * fdbMsgPtr
);

/*******************************************************************************
*   SFDB_DEV_MAC_TBL_TRIG_ACT_FUN
*
* DESCRIPTION:
*       Process triggered MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       tblActPtr   - pointer to fdb entry
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef void ( * SFDB_DEV_MAC_TBL_TRIG_ACT_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T * deviceObjPtr,
    IN GT_U8 * tblActPtr
);

/*******************************************************************************
*   SFDB_DEV_MAC_ENTRIES_AUTO_AGING_FUN
*
* DESCRIPTION:
*       invoke automatic aging process.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       data_PTR    - data of index and number of indexes that are
*                     important for the aging process.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
typedef void ( * SFDB_DEV_MAC_ENTRIES_AUTO_AGING_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T * deviceObjPtr,
    IN GT_U8 * data_PTR
);

/*******************************************************************************
*   SFDB_DEV_MAC_TBL_AGING_PROC_FUN
*
* DESCRIPTION:
*       The function is invoked by the aging process in order to send
*       the main task details about the aging (like entry index and number
*       of entries).
*
* INPUTS:
*       deviceObj   - pointer to device object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef void ( * SFDB_DEV_MAC_TBL_AGING_PROC_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T * deviceObjPtr
);

/*******************************************************************************
*   SFDB_DEV_OAM_TBL_AGING_PROC_FUN
*
* DESCRIPTION:
*       The function is invoked by the OAM aging process in order to send
*       the main task details about the aging (like entry index and number
*       of entries).
*
* INPUTS:
*       deviceObjPtr - pointer to device object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef void ( * SFDB_DEV_OAM_TBL_AGING_PROC_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T * deviceObjPtr
);

/*******************************************************************************
*   SNET_DEV_LINK_UPD_PROC_FUN
*
* DESCRIPTION:
*       Notify devices database that link state changed
*
* INPUTS:
*       deviceObjPtr - pointer to device object.
*       port         - port number.
*       linkState    - link state (0 - down, 1 - up)
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef void (* SNET_DEV_LINK_UPD_PROC_FUN ) (
                IN struct SKERNEL_DEVICE_OBJECT_T *deviceObjPtr,
                IN GT_U32                           port,
                IN GT_U32                           linkState
);

/*******************************************************************************
*   SFDB_DEV_MAC_TBL_UPLOAD_ACT_FUN
*
* DESCRIPTION:
*       Process upload MAC table engine
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       tblActPtr   - pointer to fdb entry
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef void ( * SFDB_DEV_MAC_TBL_UPLOAD_ACT_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T * deviceObjPtr,
    IN GT_U8 * tblActPtr
);

/*******************************************************************************
*   SNET_DEV_FROM_CPU_DMA_PROC_FUN
*
* DESCRIPTION:
*     Process transmitted SDMA queue frames
*
* INPUTS:
*    devObjPtr -  pointer to device object
*    bufferId  -  buffer id
*
* OUTPUT:
*
* COMMENT:
*
*******************************************************************************/
typedef void ( *SNET_DEV_FROM_CPU_DMA_PROC_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T *devObjPtr,
    IN SBUF_BUF_ID bufferId
);

/*******************************************************************************
*   SNET_DEV_FROM_EMBEDDED_CPU_PROC_FUN
*
* DESCRIPTION:
*     Process transmitted frames from the Embedded CPU to the PP
*
* INPUTS:
*    devObjPtr -  pointer to device object
*    bufferId  -  buffer id
*
* OUTPUT:
*
* COMMENT:
*
*******************************************************************************/
typedef void ( *SNET_DEV_FROM_EMBEDDED_CPU_PROC_FUN )
(
    IN struct SKERNEL_DEVICE_OBJECT_T *devObjPtr,
    IN SBUF_BUF_ID bufferId
);

/*******************************************************************************
*  SMEM_DEV_BACKUP_MEM_PROC_FUN
*
* DESCRIPTION:
*      Definition of backup/restore memory function
* INPUTS:
*       devObjPtr   - pointer to device object.
*       readWrite   - backup/restore memory data
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/

typedef void (* SMEM_DEV_BACKUP_MEM_PROC_FUN ) (
                IN struct SKERNEL_DEVICE_OBJECT_T * devObjPtr,
                IN GT_BOOL readWrite
);

/*******************************************************************************
* CPSS_DXCH_HW_IS_UNIT_USED_PROC_FUN
*
* DESCRIPTION:
*       The function checks existence of unit according to address.
*
* APPLICABLE DEVICES:
*        Lion3; Bobcat2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; Lion; xCat2; Lion2.
*
* INPUTS:
*       cpssDevNum - CPSS device number.
*       portGroupId - port group ID
*       regAddr     - register address
*
* OUTPUTS:
*       errorPtr - (pointer to) indication that function did error.
*
* RETURNS:
*       GT_TRUE  - unit is used
*       GT_FALSE - unit is not in use
*
* COMMENTS:
*
*******************************************************************************/
typedef GT_BOOL (* SMEM_GM_HW_IS_UNIT_USED_PROC_FUN ) (
                   IN GT_U32       cpssDevNum,
                   IN GT_U32       portGroupId,
                   IN GT_U32       regAddr
);

/* declaration */
struct CHT_PCL_TCAM_COMMON_DATA_STCT;

typedef void * SMAIN_SLAN_ID;

/* SLAN name size */
#define SMAIN_SLAN_NAME_SIZE_CNS                    20

/*
 * Typedef: struct SMAIN_PORT_SLAN_INFO
 *
 * Description:
 *      Describe the SLAN information for a port. The structure is used
 *      to store SLAN bind information and SLAN user_info data too.
 *
 * Fields:
 *      deviceObj    : Pointer to the device object, it's used to get SQUE id.
 *      portNumber   : Physical port number, it's used to get RX port number.
 *      buffId       : Buffer's id of Frame's data,it's used for SLAN user info.
 *      slanIdRx     : RX SLAN id.
 *      slanIdTx     : TX SLAN id.
 *      slanName     : Name of SLAN.
 *
 * Comments:
 */

typedef struct{
    SKERNEL_DEVICE_OBJECT_T_PTR deviceObj;
    GT_U32                      portNumber;
    SBUF_BUF_ID                 buffId;
    SMAIN_SLAN_ID               slanIdRx;
    SMAIN_SLAN_ID               slanIdTx;
    GT_CHAR                     slanName[SMAIN_SLAN_NAME_SIZE_CNS];
}SMAIN_PORT_SLAN_INFO;

typedef struct{
    struct SKERNEL_DEVICE_OBJECT_T *devObjPtr;/* pointer to secondary device object .
                                            used for device that embedded inside it other device ,
                                            that not really aware to the 'parent'
                                            for Example : FX950 that has FAP20M inside it

                                            use this 'port group' device that the Lion has. --> the 4 port groups of lion
                                            */
    SMEM_ACTIVE_MEM_ENTRY_STC_PTR  currentActiveMemoryEntryPtr;/*pointer to active memory that the secondary
                                device returned to the primary device , when search for memory .
                                this info should be used by the active memory wrapper of the primary device in order
                                to call the secondary device
                                for Example : FX950 that has FAP20M inside it
                                */
    GT_U32 startPortNum;        /* the port number that the device start with .
                                   for the 'sons' of the Lion , the port groups start at 0,16,32,48 */
    GT_BIT  validPortGroup;     /* indication that this port group info is valid .
                                allow to support non-exists(non-valid) port group */


}SKERNEL_CORE_DEVICE_INFO_STC;

/* SKERNEL_LTT_MAX_NUMBER_OF_PATHS_ENT
    description :
        LTT entry max number of ECMP Paths. Not applicable for QoS routing!

    values:
        SKERNEL_LTT_MAX_NUMBER_OF_PATHS_8_E -   lttNumberOfPaths = 0..7  - (xCat)
        SKERNEL_LTT_MAX_NUMBER_OF_PATHS_64_E -  lttNumberOfPaths = 0..63 - (Lion B0)
*/
typedef enum{
    SKERNEL_LTT_MAX_NUMBER_OF_PATHS_8_E = 0,
    SKERNEL_LTT_MAX_NUMBER_OF_PATHS_64_E = 1
}SKERNEL_LTT_MAX_NUMBER_OF_PATHS_ENT;


/* SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_ENT
    description :
        ECMP route entry index calculation - formula type

    values:
        SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_0_E -  <Route Entry Index> = <Base Route Entry Index> + ((packet-hash)% <Number of Paths+1>)  (xCat)
        SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_1_E -  <Route Entry Index> = <Base Route Entry Index> + Floor( (packet hash) * (Number of Paths +1) / 64 )  (Lion B0)
        SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_2_E - <Route Entry Index> = <Base Route Entry Index> +
                                                    one of next modes:
            1. Use 12 Bit Hash             packet hash * (Number of Paths +1)/4096
            2. Use 6 Lsbits of Hash        packet hash * (Number of Paths +1)/64
            3. Use 6 Msbits of Hash        packet hash * (Number of Paths +1)/64
                                                     (Lion2,Lion3)
*/
typedef enum{
    SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_0_E,
    SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_1_E,
    SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_2_E
}SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_ENT;

/* SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_ENT
    description :
        QoS route entry index calculation - formula type

    values:
        SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_0_E -  <Base Route Entry Index> + (QoSProfile-to-Route-Block-Offset(QoSProfile) % (<Number of Paths> + 1)  (xCat)
        SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_1_E -  <Base Route Entry Index> + Floor(QoSProfile-to-Route-Block-Offset(QoSProfile) * (<Number of Paths> + 1) / 8)  (Lion B0)
*/
typedef enum{
    SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_0_E,
    SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_1_E
}SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_ENT;

/*
 * Typedef: struct SKERNEL_MULTI_INSTANCE_TABLE_INFO_STC
 *
 * Description:
 *      info of table , that is duplicated on multi instances
 *
 * Fields:
*/

typedef struct{
    GT_U32  numBaseAddresses;/* number of elements pointed by multiUnitsBaseAddressPtr .
                                when 0 -> ignore multiUnitsBaseAddressPtr
                                when != 0 --> caller should ignore baseAddress and use
                                multiUnitsBaseAddressPtr[instanceIndex] --> where
                                instanceIndex is the specific index of the table.
                                */
    GT_U32  *multiUnitsBaseAddressPtr;/* when NULL - ignored .
                            when != NULL , indicate that this table is actually
                            part of unit that was duplicated , and therefore
                            'valid' on other base addresses as well !

                            the pointer is to array of base addresses , that are
                            managed by the specific 'Duplicated units'.
                            */
    GT_BOOL *multiUnitsBaseAddressValidPtr;/* when NULL - ignored.
                            when != NULL , this table is indexed like multiUnitsBaseAddressPtr[]
                            so  multiUnitsBaseAddressValidPtr[i] = GT_FALSE -->
                            result in skipping multiUnitsBaseAddressPtr[i] !
    */
}SKERNEL_MULTI_INSTANCE_TABLE_INFO_STC;

/* forward declaration */
struct SMEM_CHUNK_STCT;

/*
 * Typedef: struct SKERNEL_COMMON_TABLE_INFO_STC
 *
 * Description:
 *      common info for all tables
 *
 * Fields:
*/
typedef struct{
    char*   nameString; /* the name of the table */
    GT_U32  baseAddress;    /* base address of table */
    GT_U32  firstValidAddress;/* allow the first valid address to differ from baseAddress
                                used in validation only if firstValidAddress != 0 */

    struct SMEM_CHUNK_STCT  *memChunkPtr; /* pointer to memory that represents the table */
    SKERNEL_MULTI_INSTANCE_TABLE_INFO_STC  multiInstanceInfo;/* multi instance info */
}SKERNEL_COMMON_TABLE_INFO_STC;

/*
 * Typedef: struct SKERNEL_PARAMETER_INFO_STC
 *
 * Description:
 *      info about parameter of table
 *
 * Fields:
*/
typedef struct{
    GT_U32  step;           /* step between 2 lines in table -- in bytes */
    GT_U32  divider;        /* number of sub entries to divide with -- relevant only if not 0 */
    GT_U32  modulo;         /* modulo to operate on the access index1 , before accessing the table -- relevant only if not 0 */
    GT_U32  outOfRangeIndex;/* the first value of this parameter that considered 'out of range'
                                cause FATAL_ERROR if check fail !
                               NOTE:
                                    1. relevant only if not 0
                                    2. the operator 'modulo' applied before this check.
                               */
}SKERNEL_PARAMETER_INFO_STC;

/*
 * Typedef: struct SKERNEL_TABLE_4_PARAMETERS_INFO_STC
 *
 * Description:
 *      info of table with 4 parameter , to reduce complex macros in runtime
 *
 * Fields:
*/
typedef struct{
    SKERNEL_COMMON_TABLE_INFO_STC   commonInfo;/*common info */
    SKERNEL_PARAMETER_INFO_STC      paramInfo[4];/* 4 parameters */
}SKERNEL_TABLE_4_PARAMETERS_INFO_STC;

#define SKERNEL_TABLE_3_PARAMETERS_INFO_STC SKERNEL_TABLE_4_PARAMETERS_INFO_STC
#define SKERNEL_TABLE_2_PARAMETERS_INFO_STC SKERNEL_TABLE_4_PARAMETERS_INFO_STC
#define SKERNEL_TABLE_INFO_STC              SKERNEL_TABLE_4_PARAMETERS_INFO_STC



/* get the number of entries that the table hold , according to the memory chunk
   that is attached to it */
#define SKERNEL_TABLE_NUM_OF_ENTRIES_GET_MAC(dev,table)                                 \
    ((dev)->tablesInfo.table.commonInfo.memChunkPtr ?                                   \
        (((dev)->tablesInfo.table.commonInfo.memChunkPtr->memSize /                     \
          (dev)->tablesInfo.table.commonInfo.memChunkPtr->enrtyNumWordsAlignement)      \
          & (~1))/*clear 1 entry that was added for 'Write full entry' emulation */ :   \
      0 /* can't determine the size */                                                  \
    )


/*
 * Typedef: struct SKERNEL_TABLES_INFO_STC
 *
 * Description:
 *      info of tables , to reduce complex macros in runtime
 *
 * Fields:
*/
typedef struct{
    /* the place holder for 1 parameter tables -->
       all tables with 1 parameter must come after this place holder
        */
    GT_UINTPTR                  placeHolderFor1Parameter;/* 1 parameter place holder for Auto tests of memories */

    SKERNEL_TABLE_INFO_STC      stp;/* STP table */
    SKERNEL_TABLE_INFO_STC      mcast;/* multicast table */
    SKERNEL_TABLE_INFO_STC      vlan;/* vlan table */
    SKERNEL_TABLE_INFO_STC      ingrStc;/*ingress STC table */
    SKERNEL_TABLE_INFO_STC      egressStc;/*egress STC table */
    SKERNEL_TABLE_INFO_STC      statisticalRateLimit;/*statistical Rate limit table */
    SKERNEL_TABLE_INFO_STC      cpuCode;/*CPU code table */
    SKERNEL_TABLE_INFO_STC      qosProfile;/*QoS profile table */
    SKERNEL_TABLE_INFO_STC      fdb;/*FDB table */
    SKERNEL_TABLE_INFO_STC      portVlanQosConfig;/* Port<n> VLAN and QoS Configuration table */
    SKERNEL_TABLE_INFO_STC      pclActionTcamData;/* pcl Action Tcam Data registers */
    SKERNEL_TABLE_INFO_STC      arp;/*arp table*/
    SKERNEL_TABLE_INFO_STC      vlanPortMacSa;/* vlan/port mac SA table */
    SKERNEL_TABLE_INFO_STC      pclAction;/* pcl action table */
    SKERNEL_TABLE_INFO_STC      ipProtCpuCode;/*ip protocol cpu code table */
    SKERNEL_TABLE_INFO_STC      tunnelStart;/*tunnel start table */
    SKERNEL_TABLE_INFO_STC      tunnelStartGenericIpProfile;/* tunnel start generic ip profile table */
    SKERNEL_TABLE_INFO_STC      vrfId;/*vrf ID table */
    SKERNEL_TABLE_INFO_STC      ingressVlanTranslation;/*ingress alteration vlan translation table*/
    SKERNEL_TABLE_INFO_STC      egressVlanTranslation;/*egress alteration vlan translation table*/
    SKERNEL_TABLE_INFO_STC      macToMe;/*mac to me registers */
    SKERNEL_TABLE_INFO_STC      mll;/*mll table*/
    SKERNEL_TABLE_INFO_STC      mllOutInterfaceConfig;/*mll out interface config table*/
    SKERNEL_TABLE_INFO_STC      mllOutInterfaceCounter;/*mll out interface counter table*/
    SKERNEL_TABLE_INFO_STC      trunkNumOfMembers;/* trunk number of members table */
    SKERNEL_TABLE_INFO_STC      nonTrunkMembers;/* Non Trunk Members Table */
    SKERNEL_TABLE_INFO_STC      designatedPorts; /* Designated Port Table */
    SKERNEL_TABLE_INFO_STC      cfiUpQoSProfileSelect; /*CFI_UP To QoS-Profile Mapping Table Selector*/
    SKERNEL_TABLE_INFO_STC      ieeeTblSelect;/*IEEE_table select< 0, 1 >*/
    SKERNEL_TABLE_INFO_STC      ieeeRsrvMcCpuIndex;/*IEEE Reserved Multicast CPU Index*/
    SKERNEL_TABLE_INFO_STC      routeNextHopAgeBits; /* IPvX - Router Next Hop Age Bits Table*/
    SKERNEL_TABLE_INFO_STC      xgPortMibCounters;/* XG port mib counters */
    SKERNEL_TABLE_INFO_STC      xgPortMibCounters_1;/* additional XG port mib counters . in bobcat2 this is for ports 56..71 */

    SKERNEL_TABLE_INFO_STC      egressPolicerMeters; /* egress policer metering tables*/
    SKERNEL_TABLE_INFO_STC      egressPolicerCounters; /* egress policer counters tables*/
    SKERNEL_TABLE_INFO_STC      egressPolicerConfig;   /* sip5_15 : EPLR 'meter config' table */
    SKERNEL_TABLE_INFO_STC      egressPolicerConformanceLevelSign;/* sip5_15 : EPLR 'meter conformance level sign' tables */

    SKERNEL_TABLE_INFO_STC      policerTblAccessData; /* policer table access data*/
    SKERNEL_TABLE_INFO_STC      policerMeterPointer; /* policer port metering pointer index table */
    SKERNEL_TABLE_INFO_STC      policerIpfixWaAlert; /* IPFIX wrap around alert Memory */
    SKERNEL_TABLE_INFO_STC      policerIpfixAgingAlert; /* IPFIX aging alert Memory */
    SKERNEL_TABLE_INFO_STC      policerTimer; /* policer timer memory */
    SKERNEL_TABLE_INFO_STC      policerQos; /* policer Qos/Re-marking memory */
    SKERNEL_TABLE_INFO_STC      policerManagementCounters;/*Policer Management Counters Memory*/
    SKERNEL_TABLE_INFO_STC      policerHierarchicalQos; /* Hierarchical Policing table */

    SKERNEL_TABLE_INFO_STC      egressVlan; /* Egress Vlan Table */
    SKERNEL_TABLE_INFO_STC      egressStp; /* Egress STP Table */
    SKERNEL_TABLE_INFO_STC      l2PortIsolation; /* L2 Port Isolation Table */
    SKERNEL_TABLE_INFO_STC      l3PortIsolation; /* L3 Port Isolation Table */
    SKERNEL_TABLE_INFO_STC      sst; /* Source ID Members Table */
    SKERNEL_TABLE_INFO_STC      secondTargetPort; /* Secondary Target Port Table   */
    SKERNEL_TABLE_INFO_STC      ipclUserDefinedBytesConf; /* IPCL User Defined bytes Configuration Table */
    SKERNEL_TABLE_INFO_STC      ttiUserDefinedBytesConf;/* TTI User Defined bytes Configuration Table */
    SKERNEL_TABLE_INFO_STC      pearsonHash; /* Pearson Hash Table */
    SKERNEL_TABLE_INFO_STC      crcHashMask; /* CRC Hash Mask Memory */
    SKERNEL_TABLE_INFO_STC      tcamBistArea; /* Tcam Bist Area */
    SKERNEL_TABLE_INFO_STC      tcamArrayCompareEn; /* TCAM Array Compare Enable */
    SKERNEL_TABLE_INFO_STC      haUp0PortKeepVlan1; /* Enable keeping VLAN1 in the packet */

    SKERNEL_TABLE_INFO_STC      ttiPhysicalPortAttribute;/* tti - Physical Port Attribute Table */
    SKERNEL_TABLE_INFO_STC      ttiPhysicalPort2Attribute;/* tti - Physical Port Attribute 2 Table (sip5_20 )*/
    SKERNEL_TABLE_INFO_STC      ttiPreTtiLookupIngressEPort;/* tti - Pre-TTI Lookup Ingress ePort Table */
    SKERNEL_TABLE_INFO_STC      ttiPostTtiLookupIngressEPort;/* tti - Post-TTI Lookup Ingress ePort Table */
    SKERNEL_TABLE_INFO_STC      ttiTrillAdjacencyTcam;/* TTI - TRILL Adjacency Tcam */
    SKERNEL_TABLE_INFO_STC      ttiTrillRbid;/* TTI - TRILL RBID table*/
    SKERNEL_TABLE_INFO_STC      bridgeIngressEPort;/* Bridge - Ingress ePort Table */
    SKERNEL_TABLE_INFO_STC      bridgeIngressEPortLearnPrio;/* Bridge - Bridge Ingress ePort learn prio Table */
    SKERNEL_TABLE_INFO_STC      bridgeIngressTrunk;/* Bridge - Bridge Ingress Trunk Table */
    SKERNEL_TABLE_INFO_STC      bridgePhysicalPortEntry;/* Bridge - per physical port entry */
    SKERNEL_TABLE_INFO_STC      bridgePhysicalPortRateLimitCountersEntry;/* Bridge - per physical port - rate limit counters entry */
    SKERNEL_TABLE_INFO_STC      bridgeIngressPortMembership;/* Bridge - Ingress Port Membership Table */
    SKERNEL_TABLE_INFO_STC      ingressSpanStateGroupIndex;/* Bridge - Ingress Span State Group Index Table */

    SKERNEL_TABLE_INFO_STC      egressAndTxqIngressEcid;/* Ingress ECID - used for 802.1BR multicast source filtering */

    SKERNEL_TABLE_INFO_STC      ipvxIngressEPort;     /* IPvX - Ingress ePort Table */
    SKERNEL_TABLE_INFO_STC      ipvxIngressEVlan;     /* IPvX - Ingress eVlan Table */
    SKERNEL_TABLE_INFO_STC      ipvxQoSProfileOffsets;/* IPvX - QoS Profile Offset Table */
    SKERNEL_TABLE_INFO_STC      ipvxAccessMatrix;     /* IPvX - Access Matrix Table */
    SKERNEL_TABLE_INFO_STC      ipvxNextHop;          /* IPvX - Next Hop Table */
    SKERNEL_TABLE_INFO_STC      l2MllLtt;/*L2MLL - Lookup Translation Table (LTT).*/
    SKERNEL_TABLE_INFO_STC      eqIngressEPort;/* EQ - Ingress ePort Table */
    SKERNEL_TABLE_INFO_STC      eqL2EcmpLtt;/* EQ - L2 ecmp LTT Table */
    SKERNEL_TABLE_INFO_STC      eqTrunkLtt;/* EQ - trunk LTT Table */
    SKERNEL_TABLE_INFO_STC      eqL2Ecmp;/* EQ - L2 ecmp Table */
    SKERNEL_TABLE_INFO_STC      eqE2Phy;/* EQ - (E2PHY table) ePort to Physical Port Target Mapping Table */
    SKERNEL_TABLE_INFO_STC      txqEgressEPort;/* TXQ - Egress ePort Table */
    SKERNEL_TABLE_INFO_STC      txqDistributorDeviceMapTable;/* TXQ distributor - device map table */
    SKERNEL_TABLE_INFO_STC      haEgressEPortAttr1;/* HA - Egress ePort Table Attribute 1 */
    SKERNEL_TABLE_INFO_STC      haEgressEPortAttr2;/* HA - Egress ePort Table Attribute 2 */
    SKERNEL_TABLE_INFO_STC      haEgressPhyPort1;/* HA - Physical port Attributes table 1*/
    SKERNEL_TABLE_INFO_STC      haEgressPhyPort2;/* HA - Physical port Attributes table 2*/
    SKERNEL_TABLE_INFO_STC      haGlobalMacSa;/* HA - Global Mac Sa table */
    SKERNEL_TABLE_INFO_STC      haQosProfileToExp;/* HA - Qos Profile To Exp table */
    SKERNEL_TABLE_INFO_STC      haEpclUserDefinedBytesConfig;/* HA - EPCL User Defined Bytes Configuration table */
    SKERNEL_TABLE_INFO_STC      policerEPortEVlanTrigger;/*policer per EPort/EVlan triggering*/
    SKERNEL_TABLE_INFO_STC      l2MllVirtualPortToMllMapping;/*L2MLL - virtual port to mll mapping.*/
    SKERNEL_TABLE_INFO_STC      l2MllVidxToMllMapping;/*L2MLL - vidx to mll mapping.*/
    SKERNEL_TABLE_INFO_STC      eqLogicalPortEgressVlanMember;/*EQ - LP Egress VLAN member table*/
    SKERNEL_TABLE_INFO_STC      eqVlanMapping;/*EQ - VLAN mapping table*/
    SKERNEL_TABLE_INFO_STC      eqPhysicalPortIngressMirrorIndexTable;/*eq - Physical Port Ingress Mirror Index Table */
    SKERNEL_TABLE_INFO_STC      egfQagTargetPortMapper;/*EGF - QAG - Target Port Mapper */
    SKERNEL_TABLE_INFO_STC      egfQagEVlanDescriptorAssignmentAttributes;/*EGF - QAG - eVLAN Descriptor Assignment Attributes Table*/
    SKERNEL_TABLE_INFO_STC      egfShtVidMapper;/*EGF - SHT - vid mapper table */
    SKERNEL_TABLE_INFO_STC      egfShtEportEVlanFilter;/* EGF - SHT - Eport Vlan Filter Table*/
    SKERNEL_TABLE_INFO_STC      egfShtEgressEPort;/* EGF - SHT - Egress EPort table */
    SKERNEL_TABLE_INFO_STC      egfQagEgressEPort;/* EGF - QAG - Egress EPort table */
    SKERNEL_TABLE_INFO_STC      egfShtEVlanAttribute;/* EGF - QAG - EVlan Attribute table */
    SKERNEL_TABLE_INFO_STC      egfShtEVlanSpanning;/* EGF - QAG - EVlan Spanning table */
    SKERNEL_TABLE_INFO_STC      nonTrunkMembers2;/* EGF - SHT - non Trunk Members 2 table */
    SKERNEL_TABLE_INFO_STC      egfQagPortTargetAttribute;/* EGF - QAG - target physical Port table */
    SKERNEL_TABLE_INFO_STC      egfQagPortSourceAttribute;/* EGF - QAG - source physical Port table */
    SKERNEL_TABLE_INFO_STC      egfQagTcDpMapper;/* EGF - QAG - TC,DP table */
                                
    /* OAM Tables */
    SKERNEL_TABLE_INFO_STC      oamTable;   /* OAM table */
    SKERNEL_TABLE_INFO_STC      oamAgingTable; /* OAM aging table */
    SKERNEL_TABLE_INFO_STC      oamMegExceptionTable; /* OAM MEG Exception Table */
    SKERNEL_TABLE_INFO_STC      oamSrcInterfaceExceptionTable; /* OAM Source Interface Exception Table */
    SKERNEL_TABLE_INFO_STC      oamInvalidKeepAliveHashTable; /* OAM Invalid Keepalive Hash Table */
    SKERNEL_TABLE_INFO_STC      oamRdiStatusChangeExceptionTable; /* OAM RDI Status Change Exception Table */
    SKERNEL_TABLE_INFO_STC      oamExcessKeepAliveTable; /* OAM Excess Keepalive Table */
    SKERNEL_TABLE_INFO_STC      oamExceptionSummaryTable; /* OAM Exception Summary Table */
    SKERNEL_TABLE_INFO_STC      oamOpCodePacketCommandTable; /* OAM Opcode to Packet Command Table */
    SKERNEL_TABLE_INFO_STC      oamLmOffsetTable; /* OAM Loss Measurement Offset Table */
    SKERNEL_TABLE_INFO_STC      oamTimeStampOffsetTable; /* OAM Timestamp Offset Table */
    SKERNEL_TABLE_INFO_STC      oamTxPeriodExceptionTable; /* OAM Tx Period Exception Table */
    SKERNEL_TABLE_INFO_STC      oamProtectionLocStatusTable; /* OAM Protection LOC Status Table */
    SKERNEL_TABLE_INFO_STC      oamTxProtectionLocStatusTable;/* OAM Tx Protection LOC Status Table (sip5_20)*/

    SKERNEL_TABLE_INFO_STC      ePortToLocMappingTable; /* ePort to LOC Mapping Table */
    SKERNEL_TABLE_INFO_STC      txProtectionSwitchingTable; /* Tx Protection Switching Table */

    SKERNEL_TABLE_INFO_STC      ptpPacketCommandTable;/*Holds 3bit packet command per SrcPort (0-255) PTP_MsgType (0-15) PTP_Domain (0-4)
                                The Table holds an entry per SrcPort*/

    SKERNEL_TABLE_INFO_STC      ptpTargetPortTable; /* ERMRK PTP Target Port Table */

    SKERNEL_TABLE_INFO_STC      ERMRKQosMapTable;   /* ERMRK Qos Map Table */

    /* Lpm Tables */
    SKERNEL_TABLE_INFO_STC      lpmIpv4VrfId;
    SKERNEL_TABLE_INFO_STC      lpmIpv6VrfId;
    SKERNEL_TABLE_INFO_STC      lpmFcoeVrfId;
    SKERNEL_TABLE_INFO_STC      lpmEcmp;

    SKERNEL_TABLE_INFO_STC      tcamMemory;
    SKERNEL_TABLE_INFO_STC      globalActionTable;

    SKERNEL_TABLE_INFO_STC      epclConfigTable;/* EPCL : configuration table */
    SKERNEL_TABLE_INFO_STC      ipcl0UdbSelect;/*ipcl0 - udb selection table */
    SKERNEL_TABLE_INFO_STC      ipcl1UdbSelect;/*ipcl1 - udb selection table */
    SKERNEL_TABLE_INFO_STC      ipcl2UdbSelect;/*ipcl2 - udb selection table */
    SKERNEL_TABLE_INFO_STC      epclUdbSelect; /*epcl  - udb selection table */

    /* TXQ tables -- added for initialization of the memories , the simulation not use it runtime of packet processing */
    SKERNEL_TABLE_INFO_STC      Shared_Queue_Maximum_Queue_Limits;
    SKERNEL_TABLE_INFO_STC      Queue_Limits_DP0_Enqueue;
    SKERNEL_TABLE_INFO_STC      Queue_Buffer_Limits_Dequeue;
    SKERNEL_TABLE_INFO_STC      Queue_Descriptor_Limits_Dequeue;
    SKERNEL_TABLE_INFO_STC      Queue_Limits_DP12_Enqueue;
    SKERNEL_TABLE_INFO_STC      FC_Mode_Profile_TC_XOff_Thresholds;
    SKERNEL_TABLE_INFO_STC      CN_Sample_Intervals;
    SKERNEL_TABLE_INFO_STC      Scheduler_State_Variable;

    /* DFX server tables */
    SKERNEL_TABLE_INFO_STC      efuseFeaturesDisableBypass;
    SKERNEL_TABLE_INFO_STC      efuseDevIdBypass;
    SKERNEL_TABLE_INFO_STC      idEfuseSlave;
    SKERNEL_TABLE_INFO_STC      hdEfuseSlave;
    SKERNEL_TABLE_INFO_STC      debugUnit;

    /* TM drop */
    SKERNEL_TABLE_INFO_STC      tmDropQueueProfileId;
    SKERNEL_TABLE_INFO_STC      tmDropDropMasking;

    /* TM egress glue */
    SKERNEL_TABLE_INFO_STC      tmEgressGlueTargetInterface;

    /* BMA table */
    SKERNEL_TABLE_INFO_STC      bmaPortMapping;/*BMA - Virtual => Physical source port mapping */

    /* the place holder for 2 parameters tables -->
       all tables with single parameter must come above this line
       all tables with 2 parameters must come after this place holder
        */
    GT_UINTPTR                  placeHolderFor2Parameters;/* 2 parameters place holder for Auto tests of memories */

    SKERNEL_TABLE_2_PARAMETERS_INFO_STC pclTcam;/*pcl tcam table*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC pclTcamMask;/*pcl tcam mask table*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC tcpUdpDstPortRangeCpuCode;/*tcp Udp Dst Port Range Cpu Code table*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC routerTcam;/*router tcam table*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC ttiAction;/*tti action table */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC trunkMembers;/* trunk members table */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC deviceMapTable;/*device map table*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC policer;/*2 policer tables*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC policerCounters;/*2 policer counters tables*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC policerConfig;/* sip5_15 : 2 IPLR 'meter config' tables */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC policerConformanceLevelSign;/* sip5_15 : 2 IPLR 'meter conformance level sign' tables */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC ieeeRsrvMcConfTable;/*IEEE Reserved Multicast Configuration*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC logicalTrgMappingTable; /* Logical Target Mapping Table */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC pclConfig; /* Ingress PCL Configuration table */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC portProtocolVidQoSConf; /* Port Protocol VID and QoS Configuration Table */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC dscpToQoSProfile;/*dscp to qos profile table - instance 0..12*/
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC expToQoSProfile; /*MPLS Exp to qos profile table - instance 0..12 */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC dscpToDscpMap;/*dscp to dscp map table - instance 0..12 */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC haPtpDomain; /* (HA) PTP Domain table */

    SKERNEL_TABLE_2_PARAMETERS_INFO_STC cncMemory;/* cnc table */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC lpmMemory;/* the lpm build from 20 rams */

    /* the place holder for 3 parameters tables -->
       all tables with 2 parameters must come above this line
       all tables with 3 parameters must come after this place holder
        */
    GT_UINTPTR                  placeHolderFor3Parameters;/* 3 parameters place holder for Auto tests of memories */

    SKERNEL_TABLE_3_PARAMETERS_INFO_STC upToQoSProfile;/*up/cfi/upProfile to qos profile table  */
    SKERNEL_TABLE_3_PARAMETERS_INFO_STC ptpLocalActionTable; /* ERMRK PTP Local Action Table */
    /* the place holder for 4 parameters tables -->
       all tables with 3 parameters must come above this line
       all tables with 4 parameters must come after this place holder
        */
    GT_UINTPTR                  placeHolderFor4Parameters;/* 4 parameters place holder for Auto tests of memories */


    /* the place holder for end of all tables */
    GT_UINTPTR                  placeHolder_MUST_BE_LAST;

}SKERNEL_TABLES_INFO_STC;

struct SNET_ENTRY_FORMAT_TABLE_STCT;

/*
 * Typedef: struct SKERNEL_TABLE_FORMAT_INFO_STC
 *
 * Description: A structure to hold device tables format info.
 *
 * Fields:
 *      numFields       - number of fields in table entry
 *      fieldsInfoPtr   - pointer to table fields info
 *      fieldsNamePtr   - pointer to table fields names
 *      formatNamePtr   - pointer to table format name
 */
typedef struct
{
    GT_U32                                  numFields;
    struct SNET_ENTRY_FORMAT_TABLE_STCT *   fieldsInfoPtr;
    char **                                 fieldsNamePtr;
    char *                                  formatNamePtr;
}SKERNEL_TABLE_FORMAT_INFO_STC;

/** Typedef: enum SKERNEL_TABLE_FORMAT_ENT
 *
 * Description:
 *      Enumerator of tables formats names
 *
 *
 * Enumerations:
 *   SKERNEL_TABLE_FORMAT_TTI_DEFAULT_EPORT_E -
 *      Pre-TTI lookup ingress ePort table format
 *   SKERNEL_TABLE_FORMAT_TTI_PHYSICAL_PORT_ATTRIBUTE_E -
 *      TTI Physical port attributes table format
 *   SKERNEL_TABLE_FORMAT_TTI_EPORT_ATTRIBUTES_E -
 *      Post TTI lookup ingress table format
 *   SKERNEL_TABLE_FORMAT_VLAN_PORT_PROTOCOL_E -
 *      Vlan port protocol table format
 *   SKERNEL_TABLE_FORMAT_BRIDGE_INGRESS_EVLAN_E -
 *      Ingress VLAN table format
 *   SKERNEL_TABLE_FORMAT_BRIDGE_INGRESS_EPORT_E -
 *      Bridge ingress ePort table format
 *   SKERNEL_TABLE_FORMAT_FDB_E -
 *      FDB table format
 *   SKERNEL_TABLE_FORMAT_IP_MLL_E -
 *      IP MLL table format
 *   SKERNEL_TABLE_FORMAT_L2_MLL_E -
 *      L2 MLL table format
 *   SKERNEL_TABLE_FORMAT_EGF_QAG_EGRESS_EPORT_E -
 *      EGF_QAG - egress ePort table format
 *   SKERNEL_TABLE_FORMAT_EGF_SHT_EGRESS_EVLAN_E -
 *      Egress VLAN table format
 *   SKERNEL_TABLE_FORMAT_EGF_SHT_VLAN_ATTRIBUTES_E -
 *      egress VLAN attributes table format
 *   SKERNEL_TABLE_FORMAT_EGF_SHT_EGRESS_EPORT_E -
 *      ePort egress table format
 *   SKERNEL_TABLE_FORMAT_HA_EGRESS_EPORT_1_E -
 *      Header Alteration egress ePort table 1 format
 *   SKERNEL_TABLE_FORMAT_HA_EGRESS_EPORT_2_E -
 *      Header Alteration egress ePort table 2 format
 *   SKERNEL_TABLE_FORMAT_HA_PHYSICAL_PORT_1_E -
 *      Header Alteration physical port table 1 table format
 *   SKERNEL_TABLE_FORMAT_HA_PHYSICAL_PORT_2_E -
 *      Header Alteration physical port table 2 table format
 *   SKERNEL_TABLE_FORMAT_TUNNEL_START_E -
 *      TS table format
 *   SKERNEL_TABLE_FORMAT_HA_NAT44_E -
 *      NAT44 table format
 *   SKERNEL_TABLE_FORMAT_HA_GENERIC_TS_PROFILE_E -
 *      Generic Tunnel Start Profile table format
 *   SKERNEL_TABLE_FORMAT_IPCL_ACTION_E -
 *      IPCL action format
 *   SKERNEL_TABLE_FORMAT_EPCL_ACTION_E -
 *      EPCL action format
 *   SKERNEL_TABLE_FORMAT_FDB_AU_MSG_E -
 *      AU message format
 *   SKERNEL_TABLE_FORMAT_IPVX_ROUTER_NEXT_HOP_E -
 *      IPvX router next hop format
 *   SKERNEL_TABLE_FORMAT_TTI_ACTION_E -
 *      TTI action format
 *   SKERNEL_TABLE_FORMAT_TTI_META_DATA_E -
 *      TTI Meta Data format
 *   SKERNEL_TABLE_FORMAT_IPCL_META_DATA_E -
 *      IPCL Meta Data format
 *   SKERNEL_TABLE_FORMAT_EPCL_META_DATA_E -
 *      EPCL Meta Data format
 *   SKERNEL_TABLE_FORMAT_E2PHY_E -
 *      E2PHY format
 *   SKERNEL_TABLE_FORMAT_PLR_METERING_E -
 *      PLR metering format
 *   SKERNEL_TABLE_FORMAT_PLR_E_ATTRIBUTES_E -
 *      PLR e-attributes format
 *   SKERNEL_TABLE_FORMAT_PLR_BILLING_E -
 *      PLR billing format
 *   SKERNEL_TABLE_FORMAT_PLR_IPFIX_E -
 *      PLR IPFix format
 *   SKERNEL_TABLE_FORMAT_PLR_HIERARCHICAL_E -
 *      PLR hierarchical format
 *   SKERNEL_TABLE_FORMAT_PLR_METERING_CONFIG_E -
 *      PLR metering config format (new in sip 5_15)
 *   SKERNEL_TABLE_FORMAT_HA_NAT66_E -
 *      NAT66 table format (new in sip 5_15)
 *   SKERNEL_TABLE_FORMAT_TTI_PHYSICAL_PORT_2_ATTRIBUTE_E -
 *      TTI Physical port 2 attributes table format (new in sip 5_20)

*/
typedef enum
{
    SKERNEL_TABLE_FORMAT_TTI_DEFAULT_EPORT_E,
    SKERNEL_TABLE_FORMAT_TTI_PHYSICAL_PORT_ATTRIBUTE_E,
    SKERNEL_TABLE_FORMAT_TTI_EPORT_ATTRIBUTES_E,
    SKERNEL_TABLE_FORMAT_VLAN_PORT_PROTOCOL_E,
    SKERNEL_TABLE_FORMAT_BRIDGE_INGRESS_EVLAN_E,
    SKERNEL_TABLE_FORMAT_BRIDGE_INGRESS_EPORT_E,
    SKERNEL_TABLE_FORMAT_FDB_E,
    SKERNEL_TABLE_FORMAT_IP_MLL_E,
    SKERNEL_TABLE_FORMAT_L2_MLL_E,
    SKERNEL_TABLE_FORMAT_EGF_QAG_EGRESS_EPORT_E,
    SKERNEL_TABLE_FORMAT_EGF_SHT_EGRESS_EVLAN_E,
    SKERNEL_TABLE_FORMAT_EGF_SHT_VLAN_ATTRIBUTES_E,
    SKERNEL_TABLE_FORMAT_EGF_SHT_EGRESS_EPORT_E,
    SKERNEL_TABLE_FORMAT_HA_EGRESS_EPORT_1_E,
    SKERNEL_TABLE_FORMAT_HA_EGRESS_EPORT_2_E,
    SKERNEL_TABLE_FORMAT_HA_PHYSICAL_PORT_1_E,
    SKERNEL_TABLE_FORMAT_HA_PHYSICAL_PORT_2_E,
    SKERNEL_TABLE_FORMAT_TUNNEL_START_E,
    SKERNEL_TABLE_FORMAT_HA_NAT44_E,
    SKERNEL_TABLE_FORMAT_HA_GENERIC_TS_PROFILE_E,
    SKERNEL_TABLE_FORMAT_IPCL_ACTION_E,
    SKERNEL_TABLE_FORMAT_EPCL_ACTION_E,
    SKERNEL_TABLE_FORMAT_FDB_AU_MSG_E,
    SKERNEL_TABLE_FORMAT_IPVX_ROUTER_NEXT_HOP_E,
    SKERNEL_TABLE_FORMAT_TTI_ACTION_E,
    SKERNEL_TABLE_FORMAT_TTI_META_DATA_E,
    SKERNEL_TABLE_FORMAT_IPCL_META_DATA_E,
    SKERNEL_TABLE_FORMAT_EPCL_META_DATA_E,
    SKERNEL_TABLE_FORMAT_E2PHY_E,
    SKERNEL_TABLE_FORMAT_PLR_METERING_E,
    SKERNEL_TABLE_FORMAT_PLR_E_ATTRIBUTES_E,
    SKERNEL_TABLE_FORMAT_PLR_BILLING_E,
    SKERNEL_TABLE_FORMAT_PLR_IPFIX_E,
    SKERNEL_TABLE_FORMAT_PLR_HIERARCHICAL_E,
    /* new in sip5_15 */
    SKERNEL_TABLE_FORMAT_PLR_METERING_CONFIG_E,
    SKERNEL_TABLE_FORMAT_HA_NAT66_E,
    /* new in sip5_20 */
    SKERNEL_TABLE_FORMAT_TTI_PHYSICAL_PORT_2_ATTRIBUTE_E,
    SKERNEL_TABLE_FORMAT_EGF_QAG_PORT_TARGET_ATTRIBUTES_E,

    /* must be last */
    SKERNEL_TABLE_FORMAT_LAST_E

}SKERNEL_TABLE_FORMAT_ENT;

/*
 * Typedef: struct SKERNEL_DEVICE_MEMORY_UNIT_BASE_ADDR_STC
 *
 * Description:
 *      base addresses of memory units
 *
 * Fields:
 *      policer[3]   - base address of the {iplr0,iplr1,eplr} memory unit
 *      gts[2]       - base address of {ingress,egress} timestamp memory unit
 *      mll          - base address of MLL unit
 *      oam[2]       - base address of the {ioam,eoam} memory unit - sip5 and above
 *      CNC[2]       - base address of CNC unit(s) - sip5 and above
 *      lms[3]       - base address of LMS unit(s) - each LMS is group of ‘4 sub units’
 *                                                   in BC2 : 3 groups , other devices one group
 *                                                   0x0 base address means 0x04000000.
 *      rxDma[2]     - base address of rxDma unit(s) - sip5_15
 *      txDma[6]     - base address of txDma unit(s) - sip5_20
 *      txFifo[2]    - base address of txFifo unit(s) - sip5_15
*/
typedef struct{
    GT_U32      policer[3];
    GT_U32      gts[2];
    GT_U32      mll;

    GT_U32      oam[2]; /* sip5 and above*/
    GT_U32      CNC[2]; /* sip5 and above*/
    GT_U32      lms[3]; /* sip5 and above*/

    GT_U32      rxDma [3*2]; /* sip5_15 and above*/
    GT_U32      txDma [3*2]; /* sip5_15 and above*/
    GT_U32      txFifo[3*2];/* sip5_15 and above*/
    /* add here other units base addresses (if needed) */
}SKERNEL_DEVICE_MEMORY_UNIT_BASE_ADDR_STC;

typedef struct {
    struct SKERNEL_DEVICE_OBJECT_T *devObjPtr;      /* Pointer to device object */
    GT_TASK_HANDLE taskHandle;                      /* Keepalive transmission tasks handle */
    GT_U32 txQueue;                                 /* TxQ number */
    GT_BOOL txQueueEn;                              /* TxQ enabled */
    GT_U32 txQueuePacketGenCfgReg;                  /* Tx SDMA Packet Generator Config Queue */
    GT_U32 txQueuePacketCountCfgReg;                /* Tx SDMA Packet Count Config Queue */
    GT_U32 txQueueCounterReg;                       /* TxQ counter register */
}SKERNEL_SDMA_TRANSMIT_DATA_STC;

typedef struct {
    GT_BIT supportTasks;                                /* Device support SDMA transmission tasks */
    SKERNEL_SDMA_TRANSMIT_DATA_STC sdmaTransmitData[8]; /* SMDA transmission task data */
}SKERNEL_SDMA_SUPPORT_STC;

/*
 * Typedef: struct SKERNEL_TRAFFIC_GEN_STC
 *
 * Description:
 *      Traffic generator data used in TG task
 *
 * Fields:
 *      deviceObjPtr        - Pointer to device object
 *      trafficGenNumber    - Tragffic generator index: valid range [0..3]
 *      dataPatternPtr      - Pointer to packet's data pattern
 *      trafficGenActive    - Traffic generator state: GT_FALSE - not active
 *                                                     GT_TRUE  - active
*/
typedef struct {
    struct SKERNEL_DEVICE_OBJECT_T * deviceObjPtr;   /* Pointer to device object */
    GT_U32 trafficGenNumber;
    GT_U8 * dataPatternPtr;
    GT_BOOL trafficGenActive;
}SKERNEL_TRAFFIC_GEN_STC;

/*******************************************************************************
*   SMEM_SUB_UNIT_MEMORY_GET
*
* DESCRIPTION:
*       Get the port-group object for specific sub-unit.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address of memory(register or table).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*
*******************************************************************************/
typedef SKERNEL_DEVICE_OBJECT_T_PTR (*SMEM_SUB_UNIT_MEMORY_GET)
(
    IN SKERNEL_DEVICE_OBJECT_T_PTR devObjPtr,
    IN GT_U32                  address
);

/*******************************************************************************
*   SMEM_SPECIFIC_DEV_MEM_INIT_PART1_FUN
*
* DESCRIPTION:
*       specific part 1 of initialization that called from init 1 of 'parent device'
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*
*******************************************************************************/
typedef void (*SMEM_SPECIFIC_DEV_MEM_INIT_PART1_FUN)
(
    IN SKERNEL_DEVICE_OBJECT_T_PTR devObjPtr
);

/*******************************************************************************
*   SMEM_SPECIFIC_DEV_MEM_INIT_PART2_FUN
*
* DESCRIPTION:
*       specific part 2 of initialization that called from init 1 of 'parent device'
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*
*******************************************************************************/
typedef void (*SMEM_SPECIFIC_DEV_MEM_INIT_PART2_FUN)
(
    IN SKERNEL_DEVICE_OBJECT_T_PTR devObjPtr
);

/*******************************************************************************
*   SMEM_SPECIFIC_DEV_UNIT_ALLOC_FUN
*
* DESCRIPTION:
*       specific initialization units allocation that called before alloc units
*       of any device
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*
*******************************************************************************/
typedef void (*SMEM_SPECIFIC_DEV_UNIT_ALLOC_FUN)
(
    IN SKERNEL_DEVICE_OBJECT_T_PTR devObjPtr
);

typedef struct{
    GT_CHAR*    unitNameStr;
    GT_U32      unitNameIndex;
}SMEM_UNIT_NAME_AND_INDEX_STC;

/* entry converted from the 'Registers file' */
typedef struct{
    GT_CHAR     **unitNameStrPtr;       /* (pointer to) the name of the address of the unit*/
    GT_U32      address;                /*relative address within the unit (23 MSB)*/
    GT_U32      value;                  /*value of register*/
    GT_U32      numOfRepetitions;       /* number of repetitions*/
    GT_U32      stepSizeBeteenRepetitions;/*steps between Repetitions (value in bytes)*/

    GT_U32      numOfRepetitions_2;       /* number of repetitions*/
    GT_U32      stepSizeBeteenRepetitions_2;/*steps between Repetitions (value in bytes)*/

    GT_U32      numOfRepetitions_3;       /* number of repetitions*/
    GT_U32      stepSizeBeteenRepetitions_3;/*steps between Repetitions (value in bytes)*/
}SMEM_REGISTER_DEFAULT_VALUE_STC;
/* dummy value , that is not 'NULL' */
#define DUMMY_NAME_PTR_CNS (void*)((GT_UINTPTR)1)


/* link list of arrays of 'default registers' */
typedef struct SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STCT{
    SMEM_REGISTER_DEFAULT_VALUE_STC *currDefaultValuesArrPtr;/* pointer to current array of default values */
    struct SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STCT *nextPtr;/* pointer to element in the link list , when NULL (no next element in list) */
}SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC;

/* link list of arrays of 'duplicated units' for default registers */
typedef struct {
    GT_CHAR     *unitNameStr;       /* the name of the unit*/
    GT_U32       numOfUnits;        /* the number of next to come units that are duplicated to this one */
}SMEM_UNIT_DUPLICATION_INFO_STC;

/* macro to generate 2 lines in format of  SMEM_UNIT_DUPLICATION_INFO_STC
    for single unit that is duplicated */
#define SMEM_SINGLE_UNIT_DUPLICATION_MAC(origUnitName,dupUnitName)  \
    {STR(origUnitName),1} , {STR(dupUnitName),0}

/* macro to generate 2 lines in format of  SMEM_UNIT_DUPLICATION_INFO_STC
    for unit with 2 instances */
#define SMEM_UNIT_2_INSTANCES_MAC(unit)  \
    SMEM_SINGLE_UNIT_DUPLICATION_MAC(unit,ADD_INSTANCE_OF_UNIT_TO_ID(unit,1))
/*    {STR(unit),1} , {STR(ADD_INSTANCE_OF_UNIT_TO_ID(unit,1)),0}*/


/*******************************************************************************
*   SKERNEL_SMEM_IS_DEVICE_MEMORY_OWNER
*
* DESCRIPTION:
*       Return indication that the device is the owner of the memory.
*       relevant to multi port groups where there is 'shared memory' between port groups.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_TRUE  -  the device is     the owner of the memory.
*       GT_FALSE -  the device is NOT the owner of the memory.
*
* COMMENTS:
*
*
*******************************************************************************/
typedef GT_BOOL (*SKERNEL_SMEM_IS_DEVICE_MEMORY_OWNER)
(
    IN struct SKERNEL_DEVICE_OBJECT_T * devObjPtr,
    IN GT_U32                  address
);

typedef enum
{
     SKERNEL_REGISTERS_DB_TYPE_LEGACY_E /* using register from SMEM_CHT_MAC_REG_DB_GET(dev) (use regAddrDb) */
    ,SKERNEL_REGISTERS_DB_TYPE_SIP5_E   /* using register from SMEM_CHT_MAC_REG_DB_SIP5_GET(dev) (use regAddrDbSip5) */

    ,SKERNEL_REGISTERS_DB_TYPE_NOT_VALID_E /*indication for 'not valid' */

    ,SKERNEL_REGISTERS_DB_TYPE__LAST___E
}SKERNEL_REGISTERS_DB_TYPE_ENT;

typedef struct{
    SKERNEL_REGISTERS_DB_TYPE_ENT   registersDbType;
    GT_U32      registerOffsetInDb;/* offset from start of structure SMEM_SIP5_PP_REGS_ADDR_STC or SMEM_CHT_PP_REGS_ADDR_STC */
    GT_CHAR*    registerName;/* register's name or 'full path' */
}SKERNEL_INTERRUPT_BASIC_REG_INFO_STC;

typedef struct SKERNEL_INTERRUPT_REG_INFO_STCT{
    SKERNEL_INTERRUPT_BASIC_REG_INFO_STC    causeReg;
    SKERNEL_INTERRUPT_BASIC_REG_INFO_STC    maskReg;

    struct{
        GT_U32 myBitIndex;/* my bit index at my father's register */
        struct SKERNEL_INTERRUPT_REG_INFO_STCT* interruptPtr;/* pointer to my father (first father) */
    }myFatherInfo;
    GT_BIT isTriggeredByWrite; /* indication that interrupt must be generated
                                  on writing cause register*/
    GT_BIT isSecondFatherExists;/* indication that second father exists
           (this maybe needed for PTP per port that connected to Giga mac and to XLG mac) */
    struct{
        GT_U32 myBitIndex;/* my bit index at my second father's register */
        struct SKERNEL_INTERRUPT_REG_INFO_STCT* interruptPtr;/* pointer to my second father */
    }myFatherInfo_2;

}SKERNEL_INTERRUPT_REG_INFO_STC;

/* last line in DB of SKERNEL_INTERRUPT_REG_INFO_STC */
#define  SKERNEL_INTERRUPT_REG_INFO__LAST_LINE__CNS     \
{{/*causeReg.registersDbType*/SKERNEL_REGISTERS_DB_TYPE__LAST___E}}

/*bind reg in SMEM_SIP5_PP_REGS_ADDR_STC to interrupt */
#define SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(regPath)  \
    SKERNEL_REGISTERS_DB_TYPE_SIP5_E, /*registersDbType*/             \
    FIELD_OFFSET_IN_STC_MAC(regPath,SMEM_SIP5_PP_REGS_ADDR_STC), /*registerOffsetInDb*/ \
    #regPath

/*bind reg in SMEM_CHT_PP_REGS_ADDR_STC to interrupt */
#define SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(regPath)  \
    SKERNEL_REGISTERS_DB_TYPE_LEGACY_E, /*registersDbType*/             \
    FIELD_OFFSET_IN_STC_MAC(regPath,SMEM_CHT_PP_REGS_ADDR_STC), /*registerOffsetInDb*/ \
    #regPath

/*
 * typedef: enum SKERNEL_ADDRESS_COMPLETION_TYPE_ENT
 *
 * Description: Packet Processor address completion type
 *
 * Enumerations:
 *    SKERNEL_ADDRESS_COMPLETION_TYPE_4_REGIONS_E   - 4 regions in one register
 *           pp_address[23:0] passed as pci_address[23:0]
 *           pp_address[31:24] configured as region base (region 1-3)
 *           pci_address[25:24] contain region number.
 *           pci_address[31:26] used as PCI window base
 *
 *    SKERNEL_ADDRESS_COMPLETION_TYPE_8_REGIONS_E  - 8 regions in 8 registers
 *           pp_address[18:0] passed as pci_address[18:0]
 *           pp_address[31:19] configured as region base (region 1-7)
 *           pci_address[21:19] contain region number.
 *           pci_address[31:19] used as PCI window base
 *           (APPLICABLE DEVICES: Lion3, Bobcat2)
 *
 */
typedef enum
{
    SKERNEL_ADDRESS_COMPLETION_TYPE_4_REGIONS_E,
    SKERNEL_ADDRESS_COMPLETION_TYPE_8_REGIONS_E
}SKERNEL_ADDRESS_COMPLETION_TYPE_ENT;


/* info about the MIB counter */
typedef struct {
    GT_U32      portNum;
}SKERNEL_MIB_COUNTER_INFO_STC;


typedef struct SKERNEL_DEVICE_MEMORY_ALLOCATION_STCT{
    GT_PTR  myMemoryPtr;/* pointer to the allocation that we got from 'calloc' */
    GT_U32  myMemoryNumOfBytes;/* number of bytes of the allocation in myMemoryPtr */

    /* debug utility */
    const char*     fileNamePtr;/* file name that called the allocation */
    GT_U32          line;       /* line in the file that called the allocation */

    /* pointer to next element */
    struct SKERNEL_DEVICE_MEMORY_ALLOCATION_STCT *nextElementPtr;
}SKERNEL_DEVICE_MEMORY_ALLOCATION_STC;

/* copy info from 'replcaed' device in the new device */
#define SIM_TAKE_PARAM_FROM_OLD_DEVICE_MAC(_dev,_param) \
    _dev->_param = _dev->softResetOldDevicePtr->_param

/* info about data path (rxdma,txdma,txfifo)*/
typedef struct{
    GT_U32  dataPathFirstPort;/* first port used in the data path.*/
    GT_U32  dataPathNumOfPorts;/* number of ports in the DP .
                                if value is ZERO ... meaning 'not valid' DP .. ignored */
    GT_U32  cpuPortDmaNum;/*if 0 this DP not support the CPU DMA*/
    GT_U32  tmDmaNum;/*(egress only) if 0 this DP not support the TM DMA (traffic manager)*/
}DATA_PATH_INFO_STC;


/*
 * Typedef: struct SKERNEL_DEVICE_OBJECT
 *
 * Description:
 *      Describe a device object in the simulation.
 *
 * Fields:
 *
 * totalNumberOfBytesAllocated - total number of bytes allocated for the device.
 *                              (for statistics only ... no functionality)
 ********************************************
 * start of info for soft reset
 *      myDeviceAllocations : the list of all allocations that done by this device
 *                           (for GM devices it hold only 'simulation calloc' and without 'VERIFIER' malloc/calloc)
 *      lastDeviceAllocPtr : pointer to the last alloc that the device did
 *      softResetOldDevicePtr : pointer to the device object that used to be 'me' before the soft reset.
 *                              this pointer is not NULL only during soft reset
 *                              where need to access to some of it's info during reset
 *      softResetNewDevicePtr : pointer to the device object that will replace 'me' after soft reset is done.
 *                              this pointer is not NULL only during soft reset
 *                              needed to replace the device in threads
 *      numThreadsOnMe        : the number of threads that the device is the 'owner'
 *                               it is incremented during creation of threads
 *                               it is decremented after replacement of the 'current device' (old device)
 *                                  with new device
 *
 *      NOTE: only after numThreadsOnMe is decremented to 1 (because 'wait' in context of smainSkernelTask) ...
 *          then we can start free ALL memory associated with 'current device' (old device)
 *
 *      needToDoSoftReset   : indication that we recognized via 'active' memory
 *          that we are doing soft reset ... therefore in order to process it ASAP
 *          in the context of 'sKernel' task , we need to make buffers that
 *          currently are processed by 'sKernel' to finish ASAP
 *          NOTE: this parameter is protecting around 'while <-> sleep' that can cause
 *          'sKernel' deadlock / long time to finish.
 *
 * end of info for soft reset
 ********************************************
 *
 *      deviceId        : Null based device ID of the device's instance,
 *                      : equal to PSS Core functions deviceId parameter.
 *      deviceHwId      : Device HW Id (from AppDemo configuration)
 *      busId           : the 'deviceHwId' need to be unique on the bus that it is on.
 *                        the 'busId' allow multiple buses , so a device with same
 *                        'deviceHwId' but on different buses can be supported
 *                        ---> this was added to support PHYs on SMI 0,1 and on
 *                        several devices
 *                        for PHY busId hold 2 values : 1. the dx device  2. the SMI 0 or 1
 *                          formula is : busId = (10 * dxDevNum) + smiId --> see SMAIN_BUS_ID_MULTIPLIER_CNS
 *      deviceType      : Type of the device.
 *      deviceFamily    : the family of the device
 *      deviceRevisionId: Device revision ID (initial value is based on Silicon Stepping)
 *      deviceName      : Name of the device.
 *      devFindMemFunPtr: Find memory function pointer
 *      devIsOwnerMemFunPtr : function to state if the memory owned by the device.
 *                         see function smemIsDeviceMemoryOwner(...)
 *      devFrameProcFuncPtr
 *                      : Frame process memory function
 *      devPortLinkUpdateFuncPtr
 *                      : Link state notify changed function
 *      devFdbMsgProcFuncPtr
 *                      : FDB update message process function
 *      devMacTblTrigActFuncPtr
 *                      : Triggered MAC table entries process function
 *      devMacEntriesAutoAgingFuncPtr :
 *                      : the function implements the aging process on FDB table
 *      devFromCpuDmaFuncPtr
 *                      : Process transmitted SDMA queue frames
 *      devSfdbMacTableAgingFuncPtr :
 *                      : the function is invoked for triggered the aging process
 *      devFromEmbeddedCpuFuncPtr :
 *                      : Process transmitted frames from the Embedded CPU to the PP
 *      devMemUnitMemoryGetPtr: function to get the device that own the address.
 *      descriptorPtr   : Frame descriptor pointer
 *      registersDefaultsPtr
 *                      : Pointer to default registers data structure.
 *                      see registersDefaultsPtr_ignored for list of ignored initializations.
 *      registersDfxDefaultsPtr
 *                      : Pointer to DFX server default registers data structure.
 *      registersDefaultsPtr_ignored
 *                      : Pointer to default registers data structure , that the
 *                      registersDefaultsPtr MUST ignore !!!
 *      descrNumber     : Number of descriptors for a device
 *      descrFreeIndx   : Index of first free descriptor
 *      portsNumber     : Number MAC ports of the device
 *      numSlans        : Number of SLANs (include CPU port), size of portSlanInfo.
 *      portSlanInfo    : Pointer to the SLAN table for ports.
 *      deviceMemory    : Pointer to the device's memory object.
 *      supportActiveMemPerUnit : indication that the device supports active
 *                          memory per unit.
 *      bufPool         : Buffers pool ID of the device.
 *      queueId         : SQueue ID of the device.
 *      fdbMsgMem       : Pointer to the device's FDB updates messages memory.
 *      uplink          : type and no. of uplink
 *      tmpPeerDeviceId : tmp (because support only single device) peer device Id -- used by PHY to get MACSEC device
 *      egressBuffer    : Buffer for egress frame processing.
 *      egressBuffer1   : Buffer for egress sniffed frame processing.
 *      deviceHwId      : Physical device number,
 *                        default value 0 if not defined in the *.ini file
 *      crcBytesAdd     : Add 4 bytes to packet header for CRC
 *                        (used in RTG/RDE tests for send to CPU packets)
 *      fdbNumEntries  : fdb size (number of entries)
 *      fdbMaxNumEntries :  max number of entries in the FDB
 *      fdbNumOfBanks   : number of banks in the FDB . SIP5 : 4 / 12 / 16
 *      multiHashEnable : SIP5: is the multi hash enabled --->
 *                        0 - <multi hash enabled> not enabled --> means to use the skip bit
 *                        1 - <multi hash enabled>     enabled --> means to ignore the skip bit
 *      fdbRouteUcDeletedEntryFlag - SIP5: valid bit is 1 but the entry is counted as deleted.
 *                        [JIRA]:[MT-231] [FE-2293984] CPU NA message for deleting an entry does not
 *                         work for UC route entries
 *      fdbBankMoveMessageSupported - is the 'AU bank move message' supported by the FDB ?
 *                          Lion3 - yes , others(include Bobcat2) - no
 *      pclTcamMaxNumEntries :max number of entries in the pcl TCAM
 *      tcam_numBanksForHitNumGranularity - number of banks that Hit num granularity can be associated with.
 *                          (bc2 - 2 (banks : 0,6) , bobk : 6 (0,2,4,6,8,10) , bobcat3 - like bobcat2)
 *      supportCapwap   : do we support CAPWAP lookup on the device ? and how.
 *                        the Lion-3 may support it , but no definitions yet.
 *      supportCpuTrgPort : do we support designated port number in CPU code table
 *                          most devices has CPU port as "63" this device allow
 *                          to send "TO_CPU" to any port number -- even when
 *                          send to local CPU (not via cascade device)
 *                          This is needed for embedded CPU that is on chip.
 *      supportEmbeddedCpu : support embedded CPU (mainly for DTLS encryption/decryption)
 *      supportUdpLite : support the UDP-Lite protocol -- RFC 3828
 *                      "The Lightweight User Datagram Protocol (UDP-Lite) -- RFC 3828"
 *      embeddedCpuInfo : the info about the embedded CPU
 *                        valid when supportEmbeddedCpu == GT_TRUE or when the
 *                              deviceFamily == "embedded cpu"
 *      capwapReassemblyTablePtr - pointer to the table of CAPWAP reassembly
 *      capwapReassemblyFirstDescriptorIndex - the index in the array of
 *                          SKERNEL_DEVICE_OBJECT::descriptorPtr[] that used for
 *                          reassembly engine
 *      capwapReassemblyDescriptorsUsedBmpPtr - array of bmp that represents the
 *                          used descriptors -- each descriptor represented by
 *                          one bit.
 *      supportTunnelInterface - The device expands the standard physical port
 *                               based Bridging model to include virtual ports,
 *                               e.g. tunnel interfaces.
 *      prependNumBytes  - number of bytes to prepend before any frame that ingress.
 *                          this allow to send the original buffer when a prepend
 *                          bytes sent to CPU or other ports
 *      interruptLine   - interrupt line of the device.
 *      cmMemParamPtr   - pointer to CM(communication module) Data
 *      ipvxSupport     - support for ipvx features
 *      pclSupport     - support for PCL features:
 *                       iPclSupport5Ude - whether UDE1..UDE4 (packet types 8..11) are supported or not
 *                       iPclSupportTrunkHash - whether Trunk Hash in IPCL keys supported or not
 *                       ePclSupportVidUpTag1 - whether EPCL key fields <VID> and <UP> could refer Tag1
 *      addressCompletionType - 4 or 8 address Completion regions
 *      addressCompletionBaseShadow - address Completion shadow used only for 8 region completion
 *      tableFormatInfo - DB for devive tables formats
 *
 * Comments:
 */

typedef struct SKERNEL_DEVICE_OBJECT_T
{
    GT_U32                                      totalNumberOfBytesAllocated;
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC        myDeviceAllocations;
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC        *lastDeviceAllocPtr;
    struct SKERNEL_DEVICE_OBJECT_T              *softResetOldDevicePtr;
    struct SKERNEL_DEVICE_OBJECT_T              *softResetNewDevicePtr;
    GT_U32                                      numThreadsOnMe;
    GT_BIT                                      needToDoSoftReset;

    GT_U32                      deviceId;
    GT_U32                      deviceHwId;
    GT_U32                      busId;
    SKERNEL_DEVICE_TYPE         deviceType;
    SKERNEL_DEVICE_FAMILY_TYPE  deviceFamily;
    GT_U8                       deviceRevisionId;
    GT_CHAR                     deviceName[SKERNEL_DEVICE_NAME_MAX_SIZE_CNS];
    void *                      devFindMemFunPtr;
    SKERNEL_SMEM_IS_DEVICE_MEMORY_OWNER devIsOwnerMemFunPtr;
    SNET_DEV_FRAME_PROC_FUN     devFrameProcFuncPtr;
    SNET_DEV_LINK_UPD_PROC_FUN  devPortLinkUpdateFuncPtr;
    SFDB_DEV_MSG_PROC_FUN       devFdbMsgProcFuncPtr;
    SFDB_DEV_MAC_TBL_TRIG_ACT_FUN devMacTblTrigActFuncPtr;
    SFDB_DEV_MAC_ENTRIES_AUTO_AGING_FUN devMacEntriesAutoAgingFuncPtr;
    SNET_DEV_FROM_CPU_DMA_PROC_FUN devFromCpuDmaFuncPtr;
    SFDB_DEV_MAC_TBL_AGING_PROC_FUN devMacTblAgingProcFuncPtr;
    SFDB_DEV_MAC_TBL_UPLOAD_ACT_FUN devMacTblUploadProcFuncPtr;
    SNET_CNC_DEV_FAST_UPLOAD_FUN    devCncFastDumpFuncPtr;
    SNET_DEV_SOFT_RESET_FUN     devSoftResetFunc;
    SNET_DEV_SOFT_RESET_FUN     devSoftResetFuncPart2;
    SMEM_DEV_BACKUP_MEM_PROC_FUN devMemBackUpPtr;
    SNET_DEV_FROM_EMBEDDED_CPU_PROC_FUN devFromEmbeddedCpuFuncPtr;
    SFDB_DEV_OAM_TBL_AGING_PROC_FUN devOamTblAgingProcFuncPtr;
    SMEM_SUB_UNIT_MEMORY_GET    devMemUnitMemoryGetPtr;
    SMEM_SPECIFIC_DEV_MEM_INIT_PART1_FUN    devMemSpecificDeviceMemInitPart1;
    SMEM_SPECIFIC_DEV_MEM_INIT_PART2_FUN    devMemSpecificDeviceMemInitPart2;
    SMEM_SPECIFIC_DEV_UNIT_ALLOC_FUN        devMemSpecificDeviceUnitAlloc;

    SMEM_UNIT_NAME_AND_INDEX_STC     *devMemUnitNameAndIndexPtr;
    /* generic array of type lion2UsedUnitsAddressesArray[] */
    GT_U32   *genericUsedUnitsAddressesArray;
    /* number of entries in genericUsedUnitsAddressesArray */
    GT_U32    genericNumUnitsAddresses;

    struct {
        GT_U32  UNIT_TXQ_LL;
        GT_U32  UNIT_TXQ_QUEUE;
        GT_U32  UNIT_TXQ_DIST;
        GT_U32  UNIT_BMA;
        GT_U32  UNIT_CPFC;
        GT_U32  UNIT_DFX_SERVER;
        GT_U32  timestampBaseAddr[2];
    }specialUnitsBaseAddr;


    SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC *registersDefaultsPtr;
    SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC *registersDfxDefaultsPtr;
    SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC *registersPexDefaultsPtr;

    SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC *registersDefaultsPtr_ignored;
    SMEM_UNIT_DUPLICATION_INFO_STC            *registersDefaultsPtr_unitsDuplications;
    SMEM_UNIT_DUPLICATION_INFO_STC            *unitsDuplicationsPtr;


    void *                      descriptorPtr;
    GT_U32                      descrNumber;
    GT_U32                      descrFreeIndx;
    GT_U32                      portsNumber;
    GT_U32                      numSlans;
    SMAIN_PORT_SLAN_INFO        *portSlanInfo;
    void                   *    deviceMemory;
    GT_BIT                      supportActiveMemPerUnit;
    SBUF_POOL_ID                bufPool;
    SQUE_QUEUE_ID               queueId;
    SKERNEL_UPLINK_STC          uplink;
    GT_U32                      tmpPeerDeviceId;
    GT_U32                      gmDeviceType;
    GT_U8                       egressBuffer[SBUF_DATA_SIZE_CNS];
    GT_U8                       egressBuffer1[SBUF_DATA_SIZE_CNS];
    GT_U32                      crcBytesAdd;
    GT_U32                      calcFcsEnable;
    GT_U32                      fdbNumEntries;
    GT_U32                      fdbMaxNumEntries;
    GT_U32                      fdbNumOfBanks;
    GT_U32                      multiHashEnable;
    GT_BOOL                     fdbRouteUcDeletedEntryFlag;
    GT_U32                      *fdbBanksCounterArr[FDB_MAX_BANKS_CNS];
    GT_U32                      *fdbBanksRankArr[FDB_MAX_BANKS_CNS];
    GT_BIT                      fdbBankCounterInCpuPossess;
    GT_BIT                      fdbBankCounterUsed;
    GT_BIT                      fdbBankMoveMessageSupported;
    GT_U32                      pclTcamMaxNumEntries;
    GT_U32                      tcam_numBanksForHitNumGranularity;
    GT_U32                      oamNumEntries;
    SKERNEL_CAPWAP_LOOKUP_SUPPORT_ENT supportCapwap;
    GT_BOOL                     supportCpuTrgPort;
    GT_BOOL                     supportEmbeddedCpu;
    GT_BOOL                     supportUdpLite;
    SKERNEL_CAPWAP_REASSEMBLY_ENTRY_STC *capwapReassemblyTablePtr;
    GT_U32                      capwapReassemblyFirstDescriptorIndex;
    GT_U32                      *capwapReassemblyDescriptorsUsedBmpPtr;
    struct{
        union{
            void  *embeddedCpuDevicePtr;/* the PP has pointer to the it's embedded CPU device */
            void  *parentPpDevicePtr;/* the embedded CPU device has pointer to it's PP */
        }coupledDevice;
    }embeddedCpuInfo;
    GT_BOOL                     supportTunnelInterface;
    GT_U32                      prependNumBytes;

    GT_U32                      interruptLine;/*used only during init */
    GT_U32                      isPciCompatible;/* pci compatible(pci/pex) or smi or...*/
    SMEM_ACTIVE_MEM_ENTRY_STC_PTR activeMemPtr; /* pointer to active memory of the device */
    GT_U32                      globalInterruptCauseRegWriteBitmap_pci; /* bmp of bits that can be changed in global interrupt cause reg -- when pci device */
    GT_U32                      globalInterruptCauseRegWriteBitmap_nonPci;  /* bmp of bits that can be changed in global interrupt cause reg -- when non pci device */
    GT_U32                      rxByteSwap;/*The Rx Byte Swap Mode bit indicates whether swapping occurs from the device to the Host.
                                             0 = No swapping.
                                             1 = Byte swap: In every 32-bit word of data, the byte order is swapped so
                                                 that byte 0 is swapped with byte 3, and byte 1 is swapped with byte 2.
                                             The swapping affects the packets data only where the descriptor is always passed in Little Endian (PCI bus order).


                                             NOTE : the field called 'Rx' but it is from HOST point of view
                                             so it is actually has impact on simulation that do 'tx to cpu'

                                             */
    GT_U32                      txByteSwap;/* This bit specifies the Tx Byte Swap mode from the host to the device.
                                              0 = No swapping.
                                              1 = Byte Swap: In every 32-bit word of data, the byte order is swapped such
                                              that byte 0 is swapped with byte 3, and byte 1 is swapped with byte 2.

                                             NOTE : the field called 'Tx' but it is from HOST point of view
                                             so it is actually has impact on simulation that do 'rx from cpu'

                                              */
    GT_U32                      crcPortsBytesAdd;/* bytes adding for packet from SLAN */

    SKERNEL_DEVICE_TCAM_INFO_STC            ttiTrillAdjacencyTcanInfo;/*TTI trill Adjacency TCAM info*/
    SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC      routeTcamInfo;/*route tcam info */
    struct CHT_PCL_TCAM_COMMON_DATA_STCT    *pclTcamInfoPtr;/* pcl tcam info*/

    struct SKERNEL_DEVICE_OBJECT_T *portGroupSharedDevObjPtr;/* pointer to 'Father' device object .
                                            use by the Lion's 'port group' to have pointer to it's 'port group shared' */

    GT_BOOL                        useCoreTerminology;/* does the INI file use 'core' or 'port group' terminology .
                                                        this is 'initialization' issue only */
    SKERNEL_CORE_DEVICE_INFO_STC     *coreDevInfoPtr;/* pointer to array of 'core device' info */
    GT_U32                          numOfCoreDevs; /* number of 'core devices'*/
    GT_BOOL                         shellDevice;/* this is shell device that not do much but to hold 'sons' devices */
    GT_U32                          numOfPipes; /* number of pipes , This is multi pipe device .
                                                   using only single device object!

                                                   IMPORTANT: the 'currentPipeId'
                                                        is taken from the 'per task'
                                                        that currently serves the packet
                                                    */
    GT_U32                          numOfPortsPerPipe;/* number of ports per pipe.
                                                    relevant when 'numOfPipes' */
    GT_U32                          portGroupId;/* the port group Id (port group#) , that the port group device need in-order to convert 'local port' to global port */
    GT_MUTEX                        protectEgressMutex;/* mutex to be used by the portGroupSharedDevObjPtr device to
                                                protect the egress processing of the egress port group,
                                                since the egress port group called in the context of
                                                multi ingress port group tasks */

    GT_MUTEX                        fullPacketWalkThroughProtectMutex;/* mutex to be used by the portGroupSharedDevObjPtr device to
                                                protect the full processing of the packet

                                                this needed to allow LOG of processing of more then
                                                one core not to mix with info from other core
                                                 */

    SKERNEL_PORT_INFO_STC       portsArr[SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS];/* info about the ports */

    GT_U32                      cncNumOfUnits;/* number of CNC units : 0/1 --> means 1 , 2 means 2 */
    GT_U32                      cncBlocksNum; /* CNC blocks number */
    GT_U32                      cncBlockMaxRangeIndex; /* CNC counter block maximal range index */
    GT_U32                      cncClientSupportBitmap; /* Bitmap - CNC clients supported by device:
                                                           bit_N = 1 - mean client N supported by device
                                                           bit_N = 0 - client N not supported.
                                                           For client numeration - see SNET_CHT3_CNC_CLIENT_E */
    GT_U32                      policerEngineNum; /* Policer engines number */
    GT_BIT                      isXcatA0Features;/* relevant for xCat alike features do we support the 'A0' or 'A1 and above' */
    GT_BIT                      supportEqEgressMonitoringNumPorts;/* how many ports the device support for egress monitoring in the EQ .
                                        Lion-B , xcat2 - 64
                                        Lion2,Lion3 - 128
                                        others 32
                                          */
    GT_U32                      deviceMapTableUseDstPort;/* number of entries in the deviceMapTable is 256
                                        and not 32 , accessed by {dstDev,dstPort[3..5]}
                                        the Lion device support it */
    GT_BIT                      supportPortIsolation;/* does the device support the port isolation
                                        the xcat a1 device support it
                                        the Lion device support it */
    GT_BIT                      supportCascadeTrunkHashDstPort;/*does the device support
                                        the cascade trunk hash according to destination port
                                        the Lion device support it */
    GT_BIT                      supportOamPduTrap;  /* does the device support OAM-PDU packets trapping to CPU */
    GT_BIT                      supportPortOamLoopBack; /* does the device support OAM port loopback */

    SKERNEL_TABLES_INFO_STC     tablesInfo;/* info of tables ,
                                        to reduce complex macros in runtime */
    GT_BIT                      supportMultiAnalyzerMirroring; /*  Port-based
                                ingress mirroring allows the selection of one of
                                the seven analyzers on a per-port base for XCat */
    GT_BIT                      supportLogicalTargetMapping; /* Logical Target Mapping */
    GT_32                       ipFixTimeStampClockDiff[3]; /* Three stages clock offsets.
                                                               Used for calculation of TOD clock value. */
    GT_BIT                      supportMacSaAssignModePerPort;  /* whether MAC SA assign mode is per port
                                                                   or global */
    GT_BIT                      supportConfigurableTtiPclId;    /* whether PCL ID for the TTI keys is
                                                                   configurable in registers or constant */
    GT_BIT                      supportVlanEntryVidxFloodMode;/* whether the vidxFloodMode supported */

    GT_BIT                      supportMultiPortGroupFdbLookUpMode; /* whether Bridge FDB multi-port groups lookup supported */
    GT_BIT                      supportMultiPortGroupTTILookUpMode; /* whether TTI multi-port groups lookup supported */
    /* errata emulations */
    struct{
        GT_BIT      supportSrcTrunkToCpuIndicationEnable; /* Enable/Disable simulation of erratum - for XCat A2 and above */
        GT_BIT      srcTrunkPortIsolationAsSrcPort; /* Enable/Disable simulation of erratum - for XCat A2 and above.
                                                    When traffic from src trunk access the port isolation, it will access according to
                                                    {src port, src dev} when ingress from NON cascade port (instead of according to srcTrunk) */

        GT_BIT      srcTrunkToCpuIndication;/* Wrong Trunk-ID/Source Port Information of Packet to CPU */
        /* Reading of IEEE Reserved Multicast Configuration registers
           should be peformed from:
           0x200X820 instead of 0x200X810 (table 2)
           0x200X828 instead of 0x200X818 (table 3) - for xCat A1 and above  */
        GT_BIT      ieeeReservedMcConfigRegRead; /* JIRA: L2I-9 */
        GT_BIT      lttRedirectIndexError; /* Policy base routing LTT index can be written only to array0 of the LTT line
                                              For XCAT A1 and above */
        GT_BIT      routerGlueResetPartsOfDescriptor;/* does the router glue reset fields in the descriptor
                                              in LionA,B , xcat A0,1  */
        GT_BIT      ipfixWrapArroundFreezeMode; /* Due to errata, when working in wraparound freeze mode, if one counter reach its limit,
                                                the other counters stop counting as well, but does not count the last increment that was
                                                the reason for the freeze. */
        GT_BIT      tunnelStartPassengerEthPacketCrcRemoved;/*the device not retain the CRC of the passenger when do TS ETH ,   regardless to <Retain CRC>*/
        GT_BIT      crcTrunkHashL4InfoInIpv4Other; /* CRC trunk hash algorithm uses offsets of TCP/UDP Source and Destination ports for
                                                     IPv4 Other packets (not TCP/UDP packets).
                                                     The errata is relevant for xCat2, LionB, Lion 2*/
        GT_BIT      ttCopyToCpuWithAdditionalTag;/* packet that is TT (tunnel terminated) and will be send to CPU by ingress pipe,
                                   will be send with outer tag on top of the info in the DSA tag (similar to twice the same tag)
                                                     The errata is relevant for : ch2,3,xcat,2,lion,2,3. */
        GT_BIT      ingressPipeTrapToCpuUseNotOrig802dot1Vid;/* JIRA: HA-3259.
                                    The packet send to the CPU by the ingress pipe should send in the DSA
                                    info saved from the orig packet in decPtr->vlanTag802dot1dInfo.vid .
                                    due to this erratum the vid is updated one (not original).
                                        The errata is relevant for : bobcat2. (not to Lion3) */
        GT_BIT      mplsNonLsrNotModifyExp;/* JIRA: EQ-437.
                                            Device that suppose to allow modify EXP on MPLS non-LSR packet.
                                            but not modify due to the erratum */
        GT_BIT      unregIpmEvidxValue0;/* The eVIDX value assigned to unregistered IPv4/IPv6 MC packets (from ingress vlan entry) */
                                        /* is according to UnReg_IPM_eVIDX_Mode only without checking that Unreg_IPM_eVIDX != 0.   */
        GT_BIT      wrongEpclOamRBitReg;/* R bit value calculations in EPCL OAM related feature is based on wrong register */
                                        /* (using RDI related register). Relevant register not yet defined. */
        GT_BIT      eqNotAccessE2phyForSniffedOnDropDuplication;/* JIRA: EQ-500.
                                        EQ not access E2PHY for the Sniffed-on-Drop duplication*/
        GT_BIT      noTrafficToCpuOnBridgeBypassForwardingDecisionOnly; /* JIRA: L2I-482. */
                                                                        /* When Bridge Bypass is enabled and mode is Bypass Forwrading Decision only, */
                                                                        /* there is no traffic to CPU (this problem derive from JIRA L2I-482:         */
                                                                        /* ePort isn't applicable for packets in Bypass Forward Decision).            */
        GT_BIT      l2iApplyMoveStaticCommandRegrdlessToSecurityBreach;/* JIRA: L2I-485. */
                                                                       /* L2I : <Move Static Command > applied even when <MoveStatic Address Is SecurityBreach> = 0*/
        GT_BIT      incorrectResolutionOfTtiActionFields; /* JIRA: TTI-1114 */
                                                          /* Action resolution of the fields <eVlan>, <eVlan Cmd>, <Vid1>, <Vid1 Cmd>, <Up1 Cmd>, <Up1> is incorrect */
        GT_BIT      dsaTagOrigSrcPortNotUpdated;        /*[JIRA](TTI-1133)Original Src ePort is not updated to the newly assigned source physical port, but remains the original source ePort from the DSA tag.*/
        GT_BIT      l2echoValidityForCfmInTti;          /* JIRA: TTI-1182 */
                                                        /* CFM identification in TTI is valid only if action l2echo field is set */
        GT_BIT      llcNonSnapWithDoRouterHaCurruptL2Header;/*[JIRA] (HA-3365) LLC-nonSnap + <Do Route HA> =1 --> cause egress packet : ethertype/length=0x0(2bytes) , removing 3 bytes
                                                        NOTE: the LLC SNAP removes 8 bytes in this case : ethertype/length 2 bytes + next 6 bytes (but for LLC SNAP this is considered 'OK')*/
        GT_BIT      fdbAgingDaemonVidxEntries;/* JIRA: L2I-452 */
                                              /* Aging daemon skip fdb entries of fdb<FDBEntryType> ‘MAC’ with fdb<multiple> = 1 (vidx entries) ,
                                                    unless (<ActDevMask > & <ActDev>) = 0 (from ‘action 1,2 registers’ )
                                                 [JIRA] (L2I-452) In aging action there's no match on device although the destination is MC
                                                    */
        GT_BIT      l2MllOutMcPckCnt;/*[JIRA] (MLL-300) L2 MLL Out Multicast Packets Counter - Do not count non IP packets  */
        GT_BIT      cncHaIgnoreCrc4BytesInL2ByteCountMode;/*[JIRA] (HA-3338) HA unit Target ePort CNC client Byte Count bugs */
        GT_BIT      ttiActionNotSetModifyUpDscpWhenQosPrecedenceHard; /* [JIRA] (TTI-1265) BC2 does not allow the TTI Action to set
                                                                    ModifyUP and/or ModifyDSCP if previously assigned with QoSPrecedence==HARD */
        GT_BIT      ttiActionMayOverrideHardQosPrecedence; /* [JIRA] (TTI-1387) : Wrong resoultion of TTI Action <qos precedence> */

        GT_BIT      l2iFromCpuSubjectToL2iFiltersBridgeBypassModeIsBypassForwardingDecisionOnly;/*[JIRA] Created: Bug(L2I-514) :
                            The 'FROM_CPU' packet is subject to 'L2i filters' (on my case 'vlan not valid') when "Bridge Bypass Mode" is in mode : "Bypass Forwarding Decision only"*/
        GT_BIT      haBuildWrongTagsWhenNumOfTagsToPopIs2;/*[JIRA] (HA-3374) HA build wrong tags when <num_of_tags_to_pop> = 2*/
        GT_BIT      iplrPolicerBillingForPerPhysicalPortMeteringDoesNotWorksInFlowMode;/*[JIRA](POLICER-1322) Bobcat2 Policer Billing for Per Physical Port metering does not works In Flow Mode*/
        GT_BIT      ipLpmActivityStateDoesNotWorks; /*[JIRA] (LPM-50) LPM aging mechanism was not verified */
        GT_BIT      ipfixTodIncrementModeWraparoundIssues;/*  [JIRA] (POLICER-1370) - wrong wraparound on LSB_seconds and MSB_seconds of IPFIX TOD with operation 'increment' :
                                        1. if Wraparound on Msb needed , wrap to 1 instead of 0 (carry pass to itself !!!)
                                        2. if Wraparound on Lsb needed , carry not pass to MS word , but to itself!
                                             so wrap to 1 instead of 0
                                        */
        GT_BIT      eplrNotCountCrc;/* [JIRA] (POLICER-1304) Egress Policer byte count does not include CRC*/
        GT_BIT      txqEgressMibCountersNotClearOnRead;/* [JIRA] [TXQ-1097] - (BC2_A0) the TXQ egress mib counters should do 'clear on read' , but are not */
        GT_BIT      epclPortListModeForcePortMode;/* [JIRA]  [EPCL-443] 'EPort mode' is ignored when PortListMode = 1 So 'Port mode' used to access the EPCL configuration table. */
        GT_BIT      fastStackFailover_Drop_on_source;/* [JIRA]  [TTI-1366] Fast Stack Failover - drop_on_source packets not dropped*/
        GT_BIT      eqToCpuForRxAnalyzerSrcBasedMirrorTrgInfoInsteadOfSrcInfo;/*  [JIRA] [EQ-550] cpu port which is rx analyzer port in MirroringMode == SRC_FWD_OVERRIDE_MIRRORING_MODE
                        get 'target port,device' instead of 'source port,device'*/
        GT_BIT      ttiWrongPparsingWhenETagSecondWordEqualToEthertype;/* [JIRA] [TTI-1364] Wrong parsing when E-Tag second word equal to ethertype.
                        If E-Tag<word1>[31:16] (Re,GRP,E-CID_base[11:8]) equal to
                                Cfg<tpidX_ethertype> and
                                Cfg<tpidX_tag_type> = 8B
                            then the E-Tag is parsed as 3 words tag.
                        */
        GT_BIT      ttiTtAndPopTagNeedToBeSupportedConcurrently; /*[JIRA] : [TTI-1077] : TT and POP Tag need to be supported concurrently*/
        GT_BIT      fdbRouteUcDeleteByMsg; /*[JIRA]:[MT-231] [ERR-2293984]  :  CPU NA message for deleting an entry does not work for UC route entries */
        GT_BIT      tunnelStartQosRemarkUsePassenger; /* [JIRA]:[HA-3279] For TS packets, the Egress QoS remapping is being done according to the Passenger information */
        GT_BIT      fdbNaMsgVid1Assign; /* [JIRA]:[L2I-497] If FDB Mac SA is not found or moved and AutoLearningEnabled == 0 (i.e. controlled learning) orig_vid1 field in NA message will be 0 */
        GT_BIT      rxdmaPipIpv6WrongBitsTc;/* [JIRA]:[RXDMA-982] : PIP : the 'TC' for the IPv6 packets should be bits 116..123 of the packet but actually taken from bits 118..125 */
    }errata;

    GT_BIT                      supportRemoveVlanTag1WhenEmpty;/* enables Tag1 removal
                                from the egress port Tag State if Tag1 VID
                                is assigned a value of 0 */
    GT_BIT                      supportTxQGlobalPorts; /* TxQ unit supports global ports --
                                                        relevant to multi-core device.
                                                        Lion-A -- not support it.
                                                        Lion-B support it (also Lion2,Lion3) */
    GT_BIT                      supportForceNewDsaToCpu; /* Device supports a mode that allows packets sent to the CPU to
                                preserve the original VLAN tag (Tagging on CPU Port feature) */
    GT_BIT                      supportCrcTrunkHashMode;/* Support CRC based trunk hash mode */
    GT_BIT                      supportMacHeaderModification;/* Support MAC Header Modification */
    GT_BIT                      supportHa64Ports;/* does the device support 64 ports for
                                        egress monitoring in the HA.
                                        the Lion device support it , but still use only local ports !!!!
                                        */

    GT_BIT                      txqRevision;/* the TXQ revision .
                                            0 - all devices prior to Lion B0 (ch1,2,3,xcat,Lion A)
                                            1 - Lion B0

                                            NOTE: event newer devices then Lion B0 may still be revision 0 !!!
                                            */
    GT_BIT                      supportXgMacMibCountersControllOnAllPorts;/*does the device support
                                mac mib counter control registers , from the XG registers regardless
                                to the current port state */
    GT_BIT                      supportCutThrough; /* Cut-Through Mode Support */
    GT_BIT                      supportGemUserId; /* Support for GEM Port ID in a VLAN based system */
    GT_BIT                      supportKeepVlan1; /* Keep Vlan1 Support */
    GT_BIT                      supportResourceHistogram; /* Resource Histogram Support  */
    SKERNEL_DEVICE_MEMORY_UNIT_BASE_ADDR_STC    memUnitBaseAddrInfo;
    struct SMEM_CM_INSTANCE_PARAM_STCT  * cmMemParamPtr;
    struct SMEM_SUB20_INSTANCE_PARAM_STCT  * sub20InfoPtr; /* Pointer to SUB20 adapter info */

    /* Ipvx features support */
    struct{
        SKERNEL_LTT_MAX_NUMBER_OF_PATHS_ENT      lttMaxNumberOfPaths;   /* LTT entry max number of ECMP Paths: 8 (xCat) or 64 (Lion B0) (not applicable for QoS!); */
        SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_ENT   ecmpIndexFormula;   /* ECMP route index calc formula type */
        SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_ENT    qosIndexFormula;    /* QoS route index calc formula type */
        GT_BIT  supportPerVlanURpfMode;     /* Whether uRPF mode per Vlan supported or not */
        GT_BIT  supportPerPortSipSaCheckEnable;    /* Whether SIP/SA check can be enabled/disabled per port */
    }ipvxSupport;

    GT_BIT  supportNat44;    /* Whether device support NAT44 (sip5_10, bc2 b0 supported, sip5_15 bobK supported) */
    GT_BIT  supportNat66;    /* Whether device support NAT66 (sip 5_15 bobK supported) */

    GT_BIT  supportMaskAuFuMessageToCpuOnNonLocal;/* support the "Mask AU/FU message to CPU on non local" - supported on Lion B */
    GT_BIT  supportMaskAuFuMessageToCpuOnNonLocal_with3BitsCoreId;/* lion2 B0 : see supportMaskAuFuMessageToCpuOnNonLocal but checks 3 bits coreId and not only 2 */

    /* PCL features support */
    struct{
        GT_BIT  iPclSupport5Ude;      /* Ingress PCL - whether UDE1..UDE4 (packet type 8..11) are supported or not */
        GT_BIT  iPclSupportTrunkHash; /* Ingress PCL - whether Trunk Hash in keys supported or not */
        GT_BIT  ePclSupportVidUpTag1; /* Egress  PCL - whether EPCL key fields <VID> and <UP> could refer Tag1;
                                          0: fields <VID> and <UP> always refer Tag 0
                                          1: fields <VID> and <UP> could refer Tag0 or Tag1 as configured in register */
        GT_BIT  ipclSupportSeparatedIConfigTables; /* separate PCL config table for lookup 0-1 supported or not.
                                             0: two iPCL config tables (xCat, Lion devices)
                                                   1) for lookups 0-0 and 0-1 (shared)
                                                   2) for lookup 1
                                             1: three iPCL config tables (xCat2 device):
                                                   separate table for each lookup 0-0, 0-1, 1 */
        GT_BIT  pclSupportPortList;   /* Device support Port-List approach to bind packets to TCAM rules */
        GT_BIT  pclUseCFMEtherType;  /* Device support CFM packets identification in PCL */
    }pclSupport;

    struct{
        GT_BIT  oamSupport;     /* Device support OAM */
        GT_BIT  psuSupportPacketModification; /* Performs last minute minor packet modifications */
        GT_BIT  plrSupportOamLmCapture; /* Support LM OAM packet capturing */
        GT_BIT  keepAliveSupport; /* Support OAM Keepalive Message Processing */
        struct {
            GT_U64  keepAlivePeriodCounterTimer[8]; /* Keepalive period counter timer */
            GT_U64  keepAlivePeriodCounterValue[8]; /* Keepalive period counter value */
            GT_BIT  keepAliveAgingRequestBit[8];    /* Keepalive aging request bit */
        }keepAlivePeriodCounter[2];
    }oamSupport;

    struct {
        GT_BIT  tgSupport;      /* Device support TG */
        SKERNEL_TRAFFIC_GEN_STC trafficGenData[4]; /* Up to 4 traffic generator units per port group */
    }trafficGeneratorSupport;

    GT_U32  iPclKeyFormatVersion;/* version of IPCL key format:
                                            0 - ch2, ch3, xCat or Lion (format is different in this devices)
                                            1 - xCat2,Lion2
                                            2 - Lion3
                                             */
    GT_U32  ePclKeyFormatVersion;     /* version of EPCL key format:
                                            0 - Ch3, xCat, Lion
                                            1 - xCat2,Lion2
                                            2 - Lion3
                                            */


    GT_U32  pclTcamFormatVersion;  /* The tcam that used by the device has 52 bits comparator * 4 sub-blocks aligned to 64 bits
                                         value 0  format is 48 bit data +  4 bits control for those 52 bits  => this 4 times
                                         value 1  format is : for first sub-block : 2 bits control (entry size) + 50 bits data.
                                                       For other 3 sub-blocks : 52 bits data (no control) */
    GT_BIT                      supportDevMapTableOnOwnDev;/* support accessing to the device map table
                                            even when target device is 'ownDevice'*/
    GT_BIT  supportPtp; /* support mechanism for maintaining time synchronization between various nodes in a system */
    GT_32   todTimeStampClockDiff[2]; /* TOD counter clock delta needed for calculation of TOD clock value */

    GT_32   eTodTimeStampClockDiff[9][2];
    /* TOD auxilary pseudo registers */
    SNET_TOD_TIMER_STC  eTod[9][2];

    SKERNEL_TIMESTAMP_QUEUE_STC timestampQueue[2][TIMESTAMP_QUEUE_NUM_CNS]; /* ERMRK timestamp queues - ingress & egress, 2 queues per ingress\egress */

    /* Policer features support */
    struct{
        GT_BIT  supportPolicerMemoryControl;   /* 1 (supported) : Ingress Policer metering table, and ingress
                                                     policer counters tables  are shared between
                                                  0: feature not supported */
        GT_U32  iplrTableSize;     /* Number of entries shared for iplr0 and iplr1 (size of metering and counting tables is equal)
                                       This field is not valid if supportPolicerMemoryControl = 0 */
        GT_U32  iplr0TableSize;     /* Number of entries used by iplr0 (size of metering and counting tables is equal)
                                       This field is not valid if supportPolicerMemoryControl = 0 */
        GT_U32  iplr1TableSize;     /* Number of entries used by iplr1
                                       This field is not valid if supportPolicerMemoryControl = 0 */

        GT_U32  meterTblBaseAddr;   /* base address of metering table (in iplr0) - used for Policer Memory Control */
        GT_U32  countTblBaseAddr;   /* base address of counters table (in iplr0) - used for Policer Memory Control */
        GT_U32  meterConfigTblBaseAddr;/* new in sip5_15 : base address of metering config table (in iplr0) - used for Policer Memory Control */
        GT_U32  policerConformanceLevelSignTblBaseAddr;/* new in sip5_15 : base address of metering conformance level sign table (in iplr0) - used for Policer Memory Control */
        GT_BIT  iplr1EntriesFirst;  /* =1 when policer1 entries are placed BEFORE policer0 entries in memory (both policers have entries
                                       =0 when policer1 entries are AFTER plr0 entries, or one of policers have no entries
                                       Note: updated on Policer Memory Control mode change */
        GT_BIT supportCountingEntryFormatSelect; /* =1 - device support Counting entry format select field (Full(long) or Compressed(short))
                                                    =0 - format select not supported (always "Full" format)
                                                    see Policer Control1 register, "Counting Entry Format Select" field */
        GT_BIT supportOnlyShortCountingEntryFormat; /* Valid only if supportCountingEntryFormatSelect == 1
                                                       = 1 - only "short" counting entry format is supported  (fatal error if "full" format is configured)
                                                       = 0 - support "full" and "short" format  */
        GT_BIT supportPolicerEnableCountingTriggerByPort;   /* support "Enable Counting Trigger by Port" in
                                "Hierarchical Policer control Register"*/
        GT_BIT supportIpfixTimeStamp; /* support Policer IPFix Time stamp mechanism */
        GT_BIT supportPortModeAddressSelect;    /* This bit controls the metering entry address calculation in Port Mode.
                                                   Relevant when 'Policer Meter mode' is in Port mode only.
                                                   0=Full: The address is {index,port_num}.
                                                   1=Compressed: The address is {port_num,index}. */
        GT_BIT supportEplrPerPort;    /* there was no trigger by the 'per port'
                                         in devices before xCat2, ERLR per port does not works too */

        GT_BIT  numIplrMemoriesSupported;/* needed for 3 parts of memory for the IPLR that shared between iplr0 and iplr1(and maybe more in the future) */
        GT_U32  iplrMemoriesSize[3];/* the 3 parts of memories for the IPLR that shared between iplr0 and iplr1
                                    valid when numIplrMemoriesSupported == 1 */
        GT_U32  eplrTableSize;     /* Number of entries in eplr (size of metering and counting tables is equal) */
        GT_BIT  tablesBaseAddrSetByOrigDev;/* indication that the device already set it's own base addresses of
                                              the PLR and not need setting by 'legacy device' */
    }policerSupport;

    GT_BIT supportAnyAddress;/* does the address space supports any value as
                                'valid address' and not only those with ((address%4) == 0)
                                this to support PHY registers addresses.
                                */

    GT_BIT  supportOnlyUcRouter;    /* 1: device with only router glue layer - Cheetah 1, xCat2
                                       0: device with router engine            - Ch2, Ch3, xCat (except xCat2), Lion */
    GT_BIT  supportRegistersDb;    /* 1: device support new style of DB registers - xCat2 and above .
                                        the register addresses managed in : SMEM_CHT_MAC_REG_DB_GET(devObjPtr) */
    GT_BIT  supportFdbNewNaToCpuMsgFormat; /* 1: device support new format of NA to CPU message - xCat2 and above
                                                 see "NewNAMessageEn" field in "FDB Global Configuration" register */
    GT_BIT  supportMcTrunkHashSrcInfo; /* The index to the Designated LAG Members Table for MC packets can be
                                        calculated according to source info (VID,VIDX and OrigSourcePort/Trunk). */
    GT_BIT  supportForwardFcPackets;/* (Ch3 and above)
                                       0: Flow Control packets always terminated in the port MAC layer
                                       1: FC packets are terminated or processed by ingress pipe,
                                          depending on configuration. See 'forwardFcEnable' field
                                          in Port Serial Parameters Configuration Register */

    GT_BIT  supportForwardUnknowMacControlFrames;   /* (xCat2)
                                       0: Unknown Mac Control Frames always terminated in the port MAC layer
                                       1: Unknown Mac Control Frames are terminated or processed by ingress pipe,
                                          depending on configuration. See 'Forward Unknown MAC Control Frame' field in
                                          Port Serial Parameters 1 Configuration Register */

    GT_BIT  supportForwardPfcFrames;/*      (xCat2)
                                       0: PFC (Priority Flow Control) Frames always terminated in the port MAC layer
                                       1: PFC Frames are terminated or processed by ingress pipe,
                                          depending on configuration. See 'Forward PFC Enable' field in
                                          Port Serial Parameters 1 Configuration Register */

    GT_BIT  support1024to1518MibCounter;  /*  (xCat2)
                                    0:  Frames1024to1518Octets and Frames1519toMaxOctets MIB counters
                                        are NOT supported  (last counter is Frames1024toMaxOctets)
                                    1:  Frames1024to1518Octets and Frames1519toMaxOctets MIB counters
                                        are supported, with variable threshold between them (1518 or 1522).
                                            This counters can be used or not, depend on configuration.
                                        See 'mib4CountSelect', 'Counter 4 Limit Select' fields in
                                        port MIB Counters Control Register */

    GT_BIT  notSupportIngressPortRateLimit; /* device doesn't support per port rate limiting */

    struct{
        GT_U32  pexInt;/* bit of pex interrupt in the global interrupt cause register */
        GT_U32  pexErr;/* bit of pex error in the global interrupt cause register */
        GT_U32  cnc[2];/* bit of cnc summary in the global interrupt cause register */
        GT_U32  txqDq;
        GT_U32  txqSht;
        GT_U32  tti;
        GT_U32  tccLower;
        GT_U32  tccUpper;
        GT_U32  bcn;
        GT_U32  ha;
        GT_U32  misc;
        GT_U32  mem;
        GT_U32  txq;
        GT_U32  txqll;
        GT_U32  txqQueue;
        GT_U32  l2i;
        GT_U32  bm0;
        GT_U32  bm1;
        GT_U32  txqEgr;
        GT_U32  txqEgr1;
        GT_U32  xlgPort;
        GT_U32  fdb;
        GT_U32  ports;
        GT_U32  cpuPort;
        GT_U32  global1;
        GT_U32  txSdma;
        GT_U32  rxSdma;
        GT_U32  pcl;
        GT_U32  port24;
        GT_U32  port25;
        GT_U32  port26;
        GT_U32  port27;
        GT_U32  iplr0;
        GT_U32  iplr1;
        GT_U32  rxDma;
        GT_U32  txDma;
        GT_U32  eq;
        GT_U32  bma;
        GT_U32  eplr;
        GT_U32  ermrk;
        GT_U32  gop;
        /* new in bobcat2 */
        GT_BIT  supportMultiPorts;/* indication to support multiPorts[3]*/
        struct{
            GT_U32  globalBit;/* bit in global register */
            GT_U32  startPort;/* start port for this register*/
            GT_U32  endPort;/* end port for this register */
        }multiPorts[3];/* ports replacing <ports> and <global1> */
    }globalInterruptCauseRegister;

    SKERNEL_INTERRUPT_REG_INFO_STC      *myInterruptsDbPtr;

    GT_BIT  supportEArch;   /* support eArch : ePort , eVlan , eVidx ...
                            the lion-3 support it .

                            NOTE: this device also known as 'sip5'

                            the sip5_10 is defined by : SMEM_CHT_IS_SIP5_10_GET(dev)
                            */
    struct{
        /*
        NOTE:
            (assuming that 'first' managed sip is sip5)
            device that is 'sip5'    will have : isSip5 = 1 and other flags 0
            device that is 'sip5_10' will have : isSip5 = 1 and isSip5_10 = 1 and other flags 0
            device that is 'sip5_15' will have : isSip5 = 1 and isSip5_10 = 1 and isSip5_15 = 1 and other flags 0
            ...
        */

        GT_BIT  isSip5;     /* bobcat A0 ,Lion3 - indication that device supports sip5    features and registers structure */
        GT_BIT  isSip5_10;  /* bobcat B0        - indication that device supports sip5_10 features and registers structure */
        GT_BIT  isSip5_15;  /* bobk caelum,cetus - indication that device supports sip5_15 features and registers structure */
        GT_BIT  isSip5_20;  /* bobcat3          - indication that device supports sip5_20 features and registers structure */

    }sipRevInfo;
    GT_BIT  supportTrill; /* support TRILL : TTI unit support of TRILL
                            protocol
                            the Lion-3 support it (but currently not implemented)
                            */
    GT_U32  l2MllVersionSupport; /* l2Mll Version support: new unit similar to L3Mll replication.
                            the xCat-C0 support it - version 1.
                            the lion-3 support it  - version 2.
                            */
    GT_BIT  supportFid;   /* support Fid : lookup in FDB with FID and not VID
                            the lion-3 support it .*/
    GT_BIT  supportL2Ecmp;/* support l2 ecmp : new sub unit in the EQ.
                            the lion-3 support it .*/
    GT_U32  numWordsAlignmentInAuMsg;/* number of words alignment in AU Message format .
                            in ch1,2,3,xcat1,2,lion,Lion2 - 4 words
                            in lion3 6 words entry format - but alignment is 8 words
                            */
    GT_U32  numOfWordsInAuMsg;/* number of words in AU Message format .
                            in ch1,2,3,xcat1,2,lion,Lion2 - 4 words
                            in lion3 6 words entry format */

    GT_BIT  supportTunnelStartEthOverIpv4;/*does the TS support eth-o-Ipv4/gre
                                        supported on xcat-A3 , Lion-B0 and above */
    GT_BIT  supportTunnelstartIpTotalLengthAddValue;/*does the TS supports adding <value>
                                        to the <IP Total Length> for MacSec support
                                        supported on xcat-A3 */

    GT_BIT supportCpuInternalInterface; /* Support CPU internal interface */
    /* enable bits of eArch support. those bits updated by the 'active memory'
        mechanize . so no need to read it from the memory every packet.
    */
    GT_BIT  supportVpls;/*does the VPLS supported
                        the xcat-C0 support it */
    /* enable bits of VPLS support.
        those bits updated by the 'active memory' mechanize .
        so no need to read it from the memory every packet , and in multi-places
    */
    struct{
        GT_BIT  tti;
        GT_BIT  ipcl;
        GT_BIT  bridge;
        GT_BIT  mll;
        GT_BIT  eq;
        GT_BIT  ha;
    }vplsModeEnable;

    struct  {
        GT_U32  tableFormatVersion;/* table format version :
                            0 - table format for xcat,xcat,lion,lion2
                            1 - table format for xcat+
                            */
        GT_BIT  supportFullRange;/*does the logical map table supports full  range of {dev,port}
                        the xcat-C0 support it */
        GT_BIT  supportEgressVlanFiltering;/* does the table support egress Vlan filtering */
    }supportLogicalMapTableInfo;


    struct{
        GT_BIT  tti;
        GT_BIT  pcl[2];
        GT_BIT  bridge;
        GT_BIT  ipvx;
        GT_BIT  eq;
        GT_BIT  txq;
        GT_BIT  ha;
    }unitEArchEnable;

    /* structure for flexible length of fields between different devices */
    struct{                     /* 'old device'     'eArch max' */
        GT_U32      phyPort;    /*  6               8           */
        GT_U32      trunkId;    /*  7               12          */
        GT_U32      ePort;      /*  0               20          */
        GT_U32      eVid;       /*  12              16          */
        GT_U32      eVidx;      /*  12              16          */
        GT_U32      hwDevNum;   /*  5               12          */
        GT_U32      sstId;      /*  same as -- hwDevNum --      */
        GT_U32      stgId;      /*  8               12          */
        GT_U32      arpPtr;     /*  14              22          */
        GT_U32      tunnelstartPtr;/* 12            20          */
        GT_U32      fid;        /*  12              16          */ /* was used as VID in legacy devices */
    }flexFieldNumBitsSupport;

    struct{/* next are used when not 0 corresponding to the same resource by the
            same name in flexFieldNumBitsSupport */
        GT_U32      phyPort;/* currently for compilation purposes , may be 0 in bobk too */
        GT_U32      ePort;/* bobk supports 6k   although 'flex' 13 bits (that imply 8k) */
        GT_U32      eVid;  /* bobk supports 4.5k although 'flex' 13 bits (that imply 8k) */

        GT_U32      l2Ecmp;/* bobk supports 4k , bc2 8k */
        GT_U32      nextHop;/* bobk supports 8k , bc2 24k */
        GT_U32      mllPairs;/* bobk supports 4k , bc2 16k */
        GT_U32      l2LttMll;/* bobk supports 8k , bc2 32k */
        GT_U32      l3LttMll;/* bobk supports 4k , bc2 12k */
    }limitedResources;/* limitations when flexFieldNumBitsSupport is not enough */

    GT_BIT  supportDualDeviceID; /* does the device supports Dual Device IDs -
                                    Lion3 - support it */
    GT_BIT  designatedPortVersion; /* Extended selection of Designated Port for Multicast Packets
                                            0 - ch2, ch3, xCat, xCat2 or Lion (format is different in this devices)
                                            1 - Lion2,Lion3
                                        */
    GT_U32  numOfTxqUnits;/* number of TXQ units :
                             ch1,2,3,xcat - single TXQ unit.
                             LionB - single TXQ unit. (although it hold 4 cores)
                             Lion2,Lion3 - 2 TXQ units.

                             NOTE: Value 0,1 - means single TXQ unit.
                                   Value 2,3,4... state the number of TXQ units.
                          */

    /* enable bits of dualDeviceId support.
        those bits updated by the 'active memory' mechanize .
        so no need to read it from the memory every packet , and in multi-places
    */
    struct{
        GT_BIT  tti;
        GT_BIT  eq;
        GT_BIT  txq;
        GT_BIT  ha;
    }dualDeviceIdEnable;
    GT_U32  notSupportPciConfigMemory;/*
                                        0 - the device support access from the PCI/PEX config memory space
                                        1 (!=0) - the device NOT support access from the PCI/PEX config memory space

                                        most DXCH devices support it (even xcat2 that uses the 'Common memory' for this)
                                        DX106 (AKA DX107-B0) , Salsa devices not support it

                                        Ex/Mx devices support it
                                        Puma devices support it

                                        PHY devices not support it
                                        MACSEC devices not support it
                                        NIC devices not support it
                                        SOHO devices not support it
                                        */

    GT_U32  xgCountersStepPerPort;/* from ch3 : the address in range of 0x09000000 (ch3 and above) need to be mapped to port#
                    in ch3,xcat,lionB,Lion2,Lion3 - 0x20000 is the step of port */
    GT_U32  xgCountersStepPerPort_1;/* offsets between ports in table xgPortMibCounters_1 */
    GT_U32  offsetToXgCounters_1;/* offset from start MIB unit to table xgPortMibCounters_1 */
    GT_U32  startPortNumInXgCounters_1;/* the first port that uses table xgPortMibCounters_1 */

    GT_U32  portMacOffset;/* for generic offset between addresses of MAC ports */
    GT_U32  portMacMask;/* for generic mask for getting port number from addresses of MAC*/
    GT_U32  portMacSecondBase;/* for generic address of second base of addresses of MAC ports (relative address to the unit)*/
    GT_U32  portMacSecondBaseFirstPort;/* the first port in the of second base of addresses of MAC ports */

    SKERNEL_SDMA_SUPPORT_STC sdmaTransmitInfo;  /* SDMA transmission tasks info */

    GT_U32                   defaultEPortNumEntries;/* number of default EPort entries */

    GT_BIT                   supportStackAggregation; /* does device support stack aggregation feature.
                                                         Lion2 devices starting from B0 revisions support the feature */

    GT_BIT                   supportTxQPort14LinkUp; /* Support special treatment for port 14 in the TxQ unit.  The link is
                                                        always UP for this port in the TxQ end packets may be enqueued in this port.
                                                        The feature is in the lion2 B0 device and used for IPMC routed packets erratum WA.  */

    GT_BIT                   supportExtPortMac; /* Extended MAC support, device supports additional MACs 12 and 14.
                                                   Ports 9 and 11 may use one of two MACs according to configuration:
                                                   Port 9  - MACs 9 OR 12,
                                                   Port 11 - MACs 11 OR 14
                                                   The feature supported in Lion2  */
    GT_U32                   addressCompletionContent; /* Current content of address competion register. Relevant for Bobcat2 GM */
    SMEM_GM_HW_IS_UNIT_USED_PROC_FUN cpssDxChHwIsUnitUsedFuncPtr; /* The function checks existence of unit according to address. Relevant for Bobcat2 GM */
    GT_U32                   cpssDevNum;            /* CPSS device number. Initialized by smemGmUnitUsedFuncInit. Relevant for Bobcat2 GM */

    SKERNEL_TASK_COOKIE_INFO_STC    task_skernelCookieInfo;/* info that the sKernel task hold (for this device) */
    SKERNEL_TASK_COOKIE_INFO_STC    task_sagingCookieInfo;/* info that the sAging task hold (for this device) */
    SKERNEL_TASK_COOKIE_INFO_STC    task_oamKeepAliveAgingCookieInfo;/* info that the OAM keep alive aging task hold (for this device) */
    SKERNEL_TASK_COOKIE_INFO_STC    task_sdmaPerQueueCookieInfo[8];/* info that the SDMA tasks hold (for this device, per queue) */

    GT_U32  txqNumPorts;/* num of ports supported by TXQ (per hemisphere).
                        NOTE: value 0 --> means 64 ports
                        Bobcat2,Lion3: 72 ports
                        */

    GT_U32  txqDqNumPorts;/* num of ports supported by TXQ_DQ (per core !!!).
                        NOTE: value 0 --> means 64 ports

                        Bobcat2: 74 ports (72 ports + cpu + TM)
                        Lion3 : 33 ports (32 ports + cpu)
                        */

    GT_U32  dmaNumOfCpuPort; /* the SDMA CPU port number .
                        NOTE: value 0 --> means port 63
                        Bobcat2: DMA 72
                        Lion3 : DMA 32
                        */
    GT_U32  dmaNumOfTmPort; /* the SDMA TM port number (traffic manager).
                        Bobcat2: DMA 73
                        */

    GT_U32  cpuGopPortNumber; /* CPU MAC port NUM.
                        NOTE: value 0 --> means don't change the value already
                                          assigned, usually port 63
                        xCat3: 31
                        */

    SKERNEL_ADDRESS_COMPLETION_TYPE_ENT addressCompletionType;
    GT_U32                              addressCompletionBaseShadow[8];
    GT_BIT  cpuPortNoSpecialMac;/* is the device NOT hold special mac for the CPU port
                        0 - the device hold special mac for cpu port
                        1 - the device NOT hold special mac for cpu port (bobcat2)
                    */

    GT_U32  coreClk;        /* the core clock of the device */
    GT_U32  baseCoreClock;  /* the reference core clock for the device */
    GT_U32  fdbBaseCoreClock; /* FDB usage of the reference core clock for the device */
    GT_U32  fdbAgingFactor; /* ch3 - 16 sec , ch1,2,xcat,2,lion,2 = 10 sec, bobcat2,lion3 */

    SKERNEL_TABLE_FORMAT_INFO_STC tableFormatInfo[SKERNEL_TABLE_FORMAT_LAST_E];

    GT_BIT  supportTrafficManager;/* indication that the device hold traffic manager */
    GT_BIT  supportTrafficManager_notAllowed;/* indication that the device NOT hold traffic manager */

    struct{
        SBUF_POOL_ID                bufPool_DDR3_TM;    /* pool as if it is DDR3 memory */
    }tmInfo;/* traffic manager info */

    GT_BIT  isMsmMibOnAllPorts; /* MAC MIB counters for all ports (G included) are in the MSM - xCat3 & sip5. */
    GT_U32  unitBaseAddrAlignmentNumBits; /* Units base address in bits. */
                                          /* 0 means 23. 23 – in legacy devices (0x00800000 alignment). */
                                          /* 24 in xcat3,bc2 (0x01000000 alignment). */
    GT_BOOL  supportSplitDesignatedTrunkTable; /* support separate designated trunk tables access for UC and MC entries */
                                               /* GT_TRUE  - support separate tables (xCat3 default mode) */
                                               /* GT_FALSE - NOT support separate tables (legacy mode – all devices but xCat3) */
    GT_BIT  support802_1br_PortExtender; /* support for IEEE 802.1BR for 'Port Extender' device in the BPE system (xCat3 support it) */

    GT_BIT  supportSingleTai;/* support single TAI and not per GOP (bobk) */
    GT_U32  numOfTaiUnits;/* number of TAI units (bobcat2 is 9 , bobk is 1) */
    struct{
        GT_BIT  supportMultiDataPath;/* bobk support split RXDMA,Txdam,txFifo.. :
                                    rxdma0 - ports 0..47
                                    rxdma1 - ports 56..59 , 64..71 , 72 (CPU) , 73(TM)*/
        DATA_PATH_INFO_STC info[MAX_DP_CNS];
        GT_U32  maxDp;/* number of indexes valid in info[] */
        GT_U32  numTxqDq;/* bobk hold single DQ , but 2 rxdma,txdma,txfifo
                            bobcat3 hold (per pipe) 3 DQ,rxdma,txdma,txfifo */
        GT_BIT  supportRelativePortNum;/* Bobcat3 hold relative port number as
                                        index to the DP units.

                                        Bobk - hold GLOBAL port number !
                                        */
        GT_U32  txqDqNumPortsPerDp;/* per DP : number of 'TXQ_ports' that TXQ_DQ supports */
        GT_U32  localIndexOfDmaOfCpuPortNum;/* local index of CPU port DMA */
    }multiDataPath;

    struct{
        GT_U32      numOfLpmRams;/* number of LPM rams (that bulid the lpm memory) */
        GT_U32      perRamNumEntries;/* number of entries per ram */
        GT_U32      numOfEntriesBetweenRams;/* number of entries between 2 consecutive rams */
    }lpmRam;/* info about lpm ram */

    struct{
        GT_U32  indexInFdb; /* index in the fdb of the aging daemon .*/
        GT_BIT  daemonOnHold;/* indication that the daemon is on hold */
    }fdbAgingDaemonInfo;

    struct{
        GT_U32  indexInCnc; /* index in the CNC of the upload mechanism .*/
    }cncUploadInfo;

    GT_BIT  needResendMessage;/* indication to re-send the currently processed message ,
                                 by the skernel , to the skernel.*/

}SKERNEL_DEVICE_OBJECT;


/* Maximal number of devices
    NOTE: value need to be less then 256 to avoid GT_U8 issues
*/
#define SMAIN_MAX_NUM_OF_DEVICES_CNS                255

/* device objects database */
extern SKERNEL_DEVICE_OBJECT * smainDeviceObjects[SMAIN_MAX_NUM_OF_DEVICES_CNS];

/* check that device ID is valid in the simulation DB
NOTE: return GT_BAD_PARAM on error .

use function smemTestDeviceIdToDevPtrConvert() if fatal error is needed on error.
SKERNEL_DEVICE_OBJECT* smemTestDeviceIdToDevPtrConvert
(
    IN  GT_U32                      deviceId
);
*/
#define DEVICE_ID_CHECK_MAC(_deviceId)   \
    if ((_deviceId) >= SMAIN_MAX_NUM_OF_DEVICES_CNS || smainDeviceObjects[_deviceId] == NULL)   \
        return GT_BAD_PARAM

/* check if port 48 exists :
    simple diff between bobcat2 and bobk
*/
#define IS_PORT_48_EXISTS(dev)   \
    ((dev)->portsArr[48].state != SKERNEL_PORT_STATE_NOT_EXISTS_E)

/*check if port 0 exists :
    simple diff between bobk-Caelum,Cetus
*/
#define IS_PORT_0_EXISTS(dev)   \
    ((dev)->portsArr[0].state != SKERNEL_PORT_STATE_NOT_EXISTS_E)

/*
 * Typedef: struct SKERNEL_FRAME_DESCR_STC
 *
 * Description:
 *      Describe a descriptor of frame.
 *
 * Fields:
 *      frameBuf         : Buffer's id.
 *      dstMacPtr        : Pointer to the destination MAC.
 *      ipHeaderPtr      : Pointer to the start of IP header for IP frame.
 *      l4HeaderPtr      : Pointer to the start of L4 header for IP frame.
 *      byteCount        : Length of the frame L2 included.
 *      frameType        : Type of the frame one of SKERNEL_FRAME_TYPE
 *      srcTrunkId       : Trunk ID of the source port.
 *                       : 0 = The packet was received from a port that is
 *                       :     not a trunk member.
 *                       : 1 through 63 = The port is member of a trunk group
 *                       :
 *      srcPort          : Source port number on which the packet was received.
 *                       : Port 63 is reserved for packets sent from the CPU.
 *                       : Port 62 is reserved for packets sent by the Customer
                           Interface Bus (CIB).
 *                       :
 *      dsaTagged        : frame came with DSA tag, Set by Rx Port
 *
 *      uplink           : 0 = The packet was allocated a buffer from the port
 *                       :     buffers pool.
 *                       : 1 = The packet was allocated a buffer from the
 *                       :     Fabric/Stacking Adapter buffers pool.
 *                       :
 *      appendCrc        : Valid only when <Input_Encapsulation> = Ethernet .
 *                       : When this bit is set, a 32-bit CRC should be
 *                       : regenerated for the packets.
 *                       :
 *      mdb              : Multi-destination buffer. The buffer was allocated
 *                       : for packets that have more than one copy.
 *                       :
 *      dropPrecedence   : The packet's drop precedence..
 *                       :
 *  inputIncapsulation   : Input encapsulation:
 *                       : 0 = Ethernet
 *                       : 1 = IPv4
 *                       : 2 = MPLS
 *                       : 3 = ...
 *                       :
 *      srcVlanTagged    : Valid only if <Input_Encapsulation> = Ethernet.
 *                       : 1 = the Ethernet frame has a 802.1Q tag.
 *      dstVlanTagged    : Valid only macDaLookupResult == 1 && macDaType == 0
 *                       : 1 = the Ethernet frame has a 802.1Q tag in the egress.
 *                       :
 *      vlanCmd          : vlan_cmd
 *                       : 0 - Do not change
 *                       : 1 - Add tag
 *                       : 2 - remove tag
 *                       : 3 - modify tag
 *                       :
 *      userPriorityTag  : The 802.1p priority tag (VLAN priority tag,
 *                       :  or 802.1p User Priority) associated with the frame.
 *                       :
 *      vid              : The VLAN-ID to which the packet is classified.
 *                       : This VLAN-ID is used for the Egress filtering and
 *                       : this is the VID used if the packet is trans-mitted
 *                       : with an 802.1Q tag.
 *                       :
 *      trafficClass     : The packet's traffic class.
 *                       :
 *  controlTrap          : 0 - frame is not control neither trapped to CPU
 *                       : 1 - frame is control or trapped to CPU, send to CPU only
 *                       :
 *      pbduTrap         : 1 - frame is PBDU and should be trapped
 *        arpTrap        : 1 - frame is ARP and should be trapped
 *        igmpTrap       : 1 - frame is IGMP and should be trapped
 *                       :
 *  macSaLookupResult    : 0 - no SA entry found in the MAC table
 *                       : 1 - SA entry found in the MAC table
 *                       :
 *       macRangeCmd     : 0 - Forward
 *                       : 1 - Drop ( filtering on source address )
 *                       : 2 - Intervention on source address - Trap packet to
 *                       :     Cpu With Ingress filtering
 *                       : 3 - Control - Trap to Cpu With NO Ingress filtering
 *       saLookUpCmd     : 0 - Forward
 *                       : 1 - Drop ( filtering on source address )
 *                       : 2 - Intervention on source address - Trap packet to
 *                       :     Cpu With Ingress filtering
 *                       : 3 - Control - Trap to Cpu With NO Ingress filtering
 *       daLookUpCmd     : 0 - Forward
 *                       : 1 - Drop ( filtering on source address )
 *                       : 2 - Intervention on source address - Trap packet to
 *                       :     Cpu With Ingress filtering
 *                       : 3 - Control - Trap to Cpu With NO Ingress filtering
 *       lockPortCmd     : 0 - Forward
 *                       : 1 - Drop ( filtering on source address )
 *                       : 2 - Intervention on source address - Trap packet to
 *                       :     Cpu With Ingress filtering
 *                       : 3 - Forward + - Trap packet to
 *                       :     Cpu With Ingress filtering
 * invalidSaDrop         : 1 - drop frame because SA is multicast
 *                       :
 *    ingressFilterOut   : 1 = frame should be dropped because ingress VLAN filter
 *                       :
 *     stpState          : 0 - Disabled
 *                       : 1 - Blocking/Listening
 *                       : 2 - Learning
 *                       : 3- Forwarding
 *                       :
 *      l2Move           : 0 = no L2 movement
 *                       : 1 = L2 dynamic movement (mac entry for src mac lookup
 *                       :     is dynamic one)
 *                       : 2 = static L2 movement  (mac entry for src mac lookup
 *                       :     is static one)
 *                       :
 *    learnEn            : 0 = auto learning disable
 *                       : 1 = auto learning enable
 *                       :
 *      pktCmd           : 0 = Forward packet to destination/s according
 *                       :      to the data below
 *                       : 1 = Drop packet
 *                       : 2 = Trap to CPU
 *                       : 3 = Reserved
 *                       :
 *                       :
 *     rxSniffed         : if 1 the packet was already rx mirrored
 *                       :
 *      excludeInPort    :
 *                       :
 *      useVidx          : Indicates whether this packet should be forwarded as
 *                       : a Unicast packet or as a Multi-destination (Multicast)
 *                       : packet and whether it should be forwarded to the
 *                       : Multicast group pointed to by <VIDX>.
 *                       : 0 = The packet is Unicast.
 *                       : 1 = Use Vidx for forwarding the packet
 *                       :     (Multi-destination packet).
 *                       :
 *      cpuCode          : When <Pkt_Cmd> = Trap2CPU
 *      vidx             : When <Use_VIDX> = 1 and <Pkt_Cmd> != Trap2CPU
 *                       :
 *                       : When <Use_VIDX> = 0 and <PacketCmd> != Trap2CPU
 *      targedIsTrunk    : 1 - targedPort == trunk group id
 *                       : 0 - targedPort == port id
 *      targedPort       : The output target port number.
 *      targedDevice     : The output target device number.
 *      trunkHash        : Hash for trunk ports
 *                       :
 *  macDaLookupResult    : Valid only when <Input_Encapsulation> = Ethernet .
 *                       : 1 = The packet's MAC DA was found in the MAC table.
 *                       : 0 = The packet's MAC DA was not found in the MAC table.
 *                       :
 *      macDaType        : Valid Only when <Input_Encapsulation> = Ethernet
 *                       : 0 = Unicast MAC DA
 *                       : 1 = Multicast MAC DA
 *                       : 2 = Broadcast MAC DA
 *                       : 3 = Broadcast ARP
 *                       :
 *      flowMirrorCause  : 0 = No mirroring
 *                       : 1 = Mirroring due to RST/FIN
 *                       : 2 = Mirroring due to flow entry command
 *                       : 3 = Reserved
 *                       :
 *      doHa             : Perform header alteration on the Egress path.
 *                       : 0 = Do not alter packet's header
 *                       : 1= Perform header alteration according to <HA_Data>
 *                       :
 *      ipm              : Routed IP Multicast packet:
 *                       : 0 = The packet is not an IP Multicast routed packet.
 *                       : 1 = The packet is an IP Multicast routed packet.
 *                       :
 *      ipmPointer       : The pointer to the beginning of the IP Multicast
 *                       : forwarding linked list.
 *                       :
 *    ipmDeviceMembers   : Valid only when <IPM>=1 .
 *                       : This field maintains an index to a Multi-destination
 *                       : group to which to send this IP Multicast packet.
 *                       :
 *      srcDevice        : The packet's source device number.
 *                       :
 *      llt              : Last Level Treated
 *                       : 0 = Ethernet Bridged
 *                       : 1 = IPv4 Routed
 *                       : 2 = IPv6 Routed
 *                       : 3 = MPLS
 *                       : 4 = Reserved
 *                       : 5 = L2CE
 *                       : 6-7 = Reserved
 *                       :
 *      outLifType       : Indicates whether the outer interface is a
 *                       : Tunnel Layer (1) or Link-Layer Interface (0).
 *                       :
 *      outLinkLayer     : Valid when <Out_LIF_Type> = 0
 *                       : The Link layer for the transmitted packet:
 *                       : 0 = Ethernet
 *                       : 1 = IP (not MAC header)
 *                       : 2 = MPLS (no MAC header)
 *                       : 3 = User Define - the Out Link Layer is defined
 *                       :     according to the NLP
 *                       :
 *      ipm              : This bit is valid only when LLT = IPv4 routed.
 *                       : 0 = The packet is not routed as IP Multicast.
 *                       : 1 = The packet is routed as IP Multicast.
 *                       :
 *      sniffed          : This bit is always '0' in descriptors from the LX to
 *                       :  EPF and from EPF to Eq.
 *                       :  In descriptors going out of the Eq, this bit has
 *                       :  the following meaning:
 *                       : 1 = The descriptor is an Rx/Tx sniffer descriptor.
 *                       : 0 = The descriptor is a regular descriptor.
 *                       :
 *      flowId           : flow ID.
 *                       :
 *      decTtl           : Indicates whether to decrement the TTL
 *                       :     (and update the checksum for IPv4)
 *                       : 0 = Don't decrement TTL.
 *                       : 1 = Decrement TTL.
 *                       :
 * removeIPv4Tunnel      : Remove the IP-in-IPv4 tunnel header:
 *                       : 0 = Don't remove.
 *                       : 1 = Remove.
 *                       :
 *      mplsCmd          : 0 = NOP
 *                       : 1 = Swap
 *                       : 2 = Push 1 label
 *                       : 3 = Pop 1 label
 *                       : 4 = Pop 2 labels
 *                       : 5, 6, 7 = PoP Swap
 *                       :
 *      nlpAboveMpls     : The Network Layer Protocol above the MPLS stack.
 *                       : This field is required when popping the entire MPLS
 *                       : stack and is meaningful only when:
 *                       : 0 = IPv4
 *                       : 1 = IPv6
 *                       : 2 = MPLS
 *                       : 3 = User_Defined_Protocol_0
 *                       : 4 = User_Defined_Protocol_1
 *                       : 5 = Reserved
 *                       : 6 = L2CE, Ethernet: An Ethernet CRC must be generated
 *                       : 7 = L2CE, Non-Ethernet
 *                       :
 *      label            : Indicates the label to push or swap if
 *                       : MPLS_Command = Push or Swap.
 *                       :
 *      arpOrTunnelPrt   : If <Out_LIF_Type> == Tunnel, this is a pointer to a
 *                       :    Tunnel entry
 *                       : ELSE-IF <Out_Link_Layer> = Ethernet, this is Pointer
 *                       :    to the MAC entry that contains the MAC DA to be
 *                       :    placed in this packet when a MAC header is
 *                       :    added/overlaid.
 *                       :
 *      modifyDscpOrExp  : Indicates whether to modify (remark) the DSCP field
 *                       : in the (Outer) IPv4/IPv6 header or the EXP bit in the
 *                       : top MPLS stack:
 *                       : 0 = Don't Modify.
 *                       : 1 = Modify.
 *                       : This bit is active disregarding the Do_HA polarity.
 *                       : (even if Do HA is '1', this bit is meaningful).
 *                       :
 * copyDscpOrExpInInner  : Used in two cases only:
 *                       : Case 1: Removing an outer IPv4 tunnel header.
 *                       : Specifies whether to copy the DSCP to the inner header
 *                       : Case 2: Popping MPLS labels
 *                       : 0 = Don't Copy .
 *                       : 1 = Copy.
 *                       :
 *      dscp             : Specifies which DSCP to use for remarking the IP
 *                       : header or building a new IP Tunnel header.
 *                       :
 *      exp              : Specifies which EXP to use in remarking,
 *                       : pushing or swapping MPLS to the top.
 *                       :
 *      copyTtl          : Indicates to the HA unit what to do with TTL field
 *                       : of the inner header when popping a header
 *                       :
 *      l2ce             : Layer 2 Circuit Emulation(set only in MPLS Push
 *                         command)
 *
 *      ttl              : IF <TTL> = 0 TTL is copied from the Ip header.
 *                       : IF <TTL> !=0 set the TTL to the value defined in
 *                       :              this field.
 *                trapReservedDip         : Flag enables check of DIP is belongs to local scope
 *                         of IPMC addresses, flag set by InLif and used in
 *                         the IPv4 processing.
 *
 *                trapIntDip             : Flag enables check of DIP is belongs to reserved
 *                         addresses, flag set by InLif and used in the
 *                         IPv4 processing.
 *
 *       trapMcLocalScope : Flag enables check of DIP is belongs to local scope
 *                          of IPMC addresses, flag set by InLif and used in
 *                          the IPv4 processing.
 *
 *        trapReservedDip : Flag enables check of DIP is belongs to reserved
 *                          addresses, flag set by InLif and used in the IPv4
 *                          processing.
 *
 *        trapIntDip      : Flag enables check of DIP is belongs to internal
 *                          addresses, flag set by InLif and used in the IPv4
 *                          processing.
 *
 *        ecmpHash        : Value of ECMP hash, calculated during the IPv4
 *                          processing.
 *
 *        nextHopPtr      : Offset of next hop rout entry, calculated during the
 *                          IPv4 processing.
 *
 *    nextHopRoutMethod   : Next hop routing method.
 *
 *    nextHopEcpQosSize   : Number of bits to be taken as least significant bits
 *                          from ECP hash or InTOS.(0 = one bit, 1 = two bits ..)
 *
 *               isGIpm   : Frame should be processed as Group based IPM,
 *                          no SIP LPM.
 *
 *              isGSIpm   : Frame should be processed as Group and Source based
 *                          IPM, with SIP LPM.
 *
 *         isDefaultIpm   : IPM lookup failed - process frame according to default
 *                          IPM rout entry.
 *
 *            doIpv4Lpm   : Frame should be processed by LPM engine.
 *               lpmKey   : Search key for LPM: for unicast it's DIP for
 *                          IPM it's SIP
 *
 *       rootBucketType   : Root bucket type to start LPM search
 *
 *        nextLookupPtr   : Offset of root bucket to start LPM
 *                bvIdx   : Root bucket bit vector line index
 *
 *            lpmResult   : Result of LPM search
 *             ipv4Done   : Flags shows that frame passed by IPv4 engine
 *                          processing
 *         ttlDecrement   : if == 1 than ttl should be decrement in the egress
 *                          header alteration
 *                  ttl   : TTL value from input packet or router entry is used
 *                          to get result TTL in the egress header alteration
 *
 *       ipv4CounterSet   : Value 0; 1; or 2 - define set of ipv4 counters
 *                          for frames passed through ipv4 engine,
 *                          value 3 - means no counters are updated
 *            oulifType   : 0 - LogicalLink Layer; 1 - tunnel; 2 - MLL
 *
 *           LLL_outLif   :
 *           arpPointer   :
 *                  mll   : mll used when outLifType == 2
 *
 *           deviceVidx   : used when outLifType == 2
 *
 *           tcbDidDrop   : the TCB block caused a drop action
 *
 *           traffCondOffset : offset to traffic condition entry from start of TC table
 *
 *           activateTC   : do metering(traffic condition)
 *
 *           activateCount: do counting(traffic condition)
 *
 *           portProtMatch: Protocol was matched for port
 *
 *           portProtTc:    Traffic Class to set according to matched protocol
 *
 *           forceL3Cos:    ForceL3Cos Register was set or not
 *
 *           saDaTc:        If SA or DA were found then TC is maximal value
 *                          of both entries, if not then the value is 0xff
 *
 *           privEdgeVlanEn: True if privEdgeVlan is enabled in the system
 *                             is enabled for port
 *           extendTxPktPtr: Pointer to extended transmit packet information
 *
 *
 *******************************************************************************
 *   EXTRA SUPPORT FOR SAMBA
 *           pclId0       : used as part of key for first lookup in pcl TCAM
 *
 *           pclId1       : used as part of key for second lookup in pcl TCAM
 *
 *           doPcl        : used as a condition to enter the Pcl Engine
 *
 *******************************************************************************
 *
 * Comments:
 */
typedef struct {
    /* Word 0 */
    SBUF_BUF_ID                 frameBuf;
    GT_U8                       *dstMacPtr;
    GT_U8                       *ipHeaderPtr;
    GT_U8                       *l4HeaderPtr;
    GT_U16                      byteCount;
    GT_U16                      frameType;
    GT_U16                      srcTrunkId;
    GT_U32                      srcPort;
    GT_BIT                      dsaTagged;
    GT_U32                      uplink;
    GT_U8                       appendCrc;
    GT_U8                       mdb;
    GT_U8                       dropPrecedence;
    GT_U8                       inputIncapsulation;
    GT_U8                       srcVlanTagged;
    GT_U8                       dstVlanTagged;
    GT_U8                       vlanCmd;
    GT_U8                       userPriorityTag;
    GT_U16                      vid;
    GT_U8                       trafficClass;
    GT_U8                       controlTrap;
    GT_U8                       pbduTrap;
    GT_U8                       arpTrap;
    GT_U8                       igmpTrap;
    GT_U8                       macRangeTrap;
    GT_U8                       macRangeDrop;
    GT_U8                       macSaLookupResult;
    GT_U8                       macRangeCmd;
    GT_U8                       saLookUpCmd;
    GT_U8                       daLookUpCmd;
    GT_U8                       islockPort;
    GT_U8                       lockPortCmd;
    GT_U8                       invalidSaDrop;
    GT_U8                       ingressFilterOut;
    GT_U8                       stpState;
    GT_U8                       l2Move;
    GT_U8                       learnEn;
    GT_U8                       pktCmd;
    GT_U8                       rxSniffed;
    GT_U8                       txSniffed;
    /* Word 1 */
    GT_U8                       excludeInPort;
    GT_U8                       useVidx;
    union {
        struct{
            GT_U16              cpuCode;
        }cmd_trap2cpu;
        struct{
            GT_U16              vidx;
        }useVidx_1;
        struct{
            GT_U8              targetIsTrunk;
            GT_U8              targedPort;
            GT_U8              targedDevice;
            GT_U8              trunkHash;
        }useVidx_0;
    }bits15_2;
    GT_U8                      macDaLookupResult;
    GT_U8                      macDaType;
    GT_U8                      flowMirrorCause;
    GT_U8                      flowTemplate;
    SKERNEL_FLOW_REDIRECT_ENT  flowRedirectCmd;
    GT_U32                     flowNhlfeOffset;
    GT_U8                      flowCmd;
    GT_U8                      doHa;
    GT_U32                     ipv4SIP; /* ipv4 source IP address */
    GT_U32                     ipv4DIP; /* ipv4 destination IP address*/
    GT_U8                      doRout;
    GT_U8                      doClassify;
    GT_U8                      ipm;
    void                       *ipmPointer;
    GT_U16                     ipmDeviceMembers;
    GT_U8                      srcDevice;
    GT_U32                     vidHasNoLocalMember;
    GT_U32                     vidHasNoUplinkMember;
    /* HA_DATA */
    GT_U8                      llt;
    GT_U8                      outLifType;
    GT_U8                      outLinkLayer;
    GT_U8                      outTunnelLayer;
    GT_U8                      ipm_1;
    GT_U8                      sniffed;
    GT_U16                     flowId;
    GT_U8                      decTtl;
    GT_U8                      removeIPv4Tunnel;
    GT_U8                      mplsCmd;
    GT_U8                      nlpAboveMpls;
    GT_U32                     label;
    void *                     arpOrTunnelPrt;
    GT_U8                      modifyDscpOrExp;
    GT_U8                      dscp;
    GT_U8                      exp;
    GT_U8                      copyTtl;
    GT_U8                      l2ce;
    /* InLif Data */
    GT_U8                      majorTemplate;
    GT_U8                      setVrId;
    GT_U16                     vrId;
    GT_U16                     inLifNumber;
    GT_U16                     pclBaseAddr;
    GT_U16                     pclMaxHops;
    GT_U16                     pclNum;
    SKERNEL_TIGER_INLIF_CALSSIFICATION_TYPE_ENT   inLifClassify;
    /* extra support for tiger    */
    GT_U16                     doMirrorToCpu;


    /* Additions for IPv4 support */
    GT_U8                      trapMcLocalScope;
    GT_U8                      trapReservedDip;
    GT_U8                      trapIntDip;
    GT_U16                     ecmpHash;
    GT_U32                     nextHopPtr;
    POLICY_SWITCH_TYPE_ENT     policySwitchType;
    IPV4_LPM_ROUT_METHOD_ENT   nextHopRoutMethod;
    GT_U8                      nextHopEcpQosSize;
    GT_U8                      isGIpm;
    GT_U8                      isGSIpm;
    GT_U8                      isDefaultIpm;
    GT_U8                      doIpv4Lpm;
    GT_U8                      doIpmRout;
    GT_U32                     lpmKey;
    /* extra support for tiger    */
    GT_U8                      enReservedDip;       /* tiger only */
    GT_U8                      enRestrictedDip;     /* tiger only */
    GT_U8                      trapReservedSip;     /* tiger only */
    GT_U8                      trapRestrictedSip;   /* tiger only */
    GT_U8                      mcLocalScopeEn;      /* tiger only */
    GT_U8                      ipV6site;            /* tiger only */
    GT_U8                      ipv6Icmp;            /* tiger only */
    GT_U8                      igmpDetect;          /* tiger only */
    GT_U8                      defNextHop;          /* tiger only */
    GT_U8                      ipV4Mgmt;            /* tiger only */
    GT_U8                      ipV6Mgmt;            /* tiger only */
    GT_U8                      ipv4Ripv1;           /* tiger only */

    IPV4_LPM_BUCKET_TYPE_ENT   rootBucketType;
    GT_U32                     nextLookupPtr;
    GT_U8                      bvIdx;

    IPV4_LPM_RESULT_ENT        lpmResult;
    GT_U8                      ipv4Done;
    GT_U8                      ttlDecrement;
    GT_U8                      ttl;
    GT_U8                      ipv4CounterSet;
    GT_U32                     LLL_outLif;
    GT_U32                     arpPointer;
    GT_U32                     mll;
    GT_U32                     deviceVidx;
    GT_U32                     Tunnel_outLif0To31;
    GT_U16                     Tunnel_outLif32To41;

    /* traffic condition support */
    GT_BOOL                    tcbDidDrop;
    GT_U16                     traffCondOffset;
    GT_BOOL                    activateTC;
    GT_BOOL                    activateCount;

    /* extra support for Samba and tiger */
    GT_U32                     pclId0;
    GT_U32                     pclId1;
    GT_U8                      doPcl;
    GT_U8                      pclProfile;
    GT_U8                      pclLongLkup0;
    GT_U8                      pclLongLkup1;

    /* MPLS switching support */
    GT_U8                      doMpls;
    GT_U8                      *mplsTopLablePtr;
    MPLS_LABELINFO_STC         mplsLabels[2];
    GT_U8                      mplsDone;
    GT_U8                      mplsMtuExceed;
    GT_U32                     mplsPtr; /* pointer to an hnlfe */

    /* CoS decisions */
    GT_BOOL                    portProtMatch;
    GT_U8                      portProtTc;
    GT_U8                      forceL3Cos;
    GT_U8                      saDaTc;

    /* Private Edge Vlan Port Enable */
    GT_BOOL                    privEdgeVlanEn;

    /* Pointer to Transmit Packet Information */
    GT_U32                   * extendTxPktPtr;
}SKERNEL_FRAME_DESCR_STC;

/*
 * Typedef: struct SKERNEL_FRAME_SOHO_DESCR_STC
 *
 * Description:
 *      Describe a descriptor of frame.
 *
 * Fields:
 *      frameBuf         : Buffer's id.
 *      byteCount        : Length of the frame L2 included.
 *      dstMacPtr        : Pointer to the destination MAC.
 *      ipHeaderPtr      : Pointer to the start of IP header for IP frame.
 *      srcPort          : Source port number on which the packet was received.
 *      vid              : The VLAN-ID to which the packet is classified.
 *                       : This VLAN-ID is used for the Egress filtering and
 *                       : this is the VID used if the packet is trans-mitted
 *                       : with an 802.1Q tag.
 *      useDaPriority    : Use DA priority for this frame
 *      daPriority       : DA priority
 *      useVtuPriority   : Use VTU priority for this frame
 *      vtuPriority      : VTU priority
 *      useSaPriority    : Use SA priority for this frame
 *      saPriority       : SA priority
 *      useIpvxPriority  : Use IPVX priority
 *      ipPriority       : IP priority
 *      ieeePiority      : IEEE priority
 *      mld              : DA MAC type is MLD
 *      igmp             : DA MAC type is IGMP
 *      pause            : DA MAC type is PAUSE
 *      igmpSnoop        : IGMP snooping
 *      destPortVector   : Destination ports vector
 *      notRateLimit     :
 *      dbNum            : MAC address data base number
 *      saHit            : SA found in forwarding database table flag
 *      saUpdate         : SA need to be updated flag
 *      daHit            : DA found in forwarding database table flag
 *      vtuHit           : VTU found in forwarding database table flag
 *      vtuMiss          : Vlan ID not found in VTU table flag
 *      spOk             : Source port is OK
 *      vtuVector        : Vlan table ports members
 *      Mgmt             : Frame should be managed with Marvell Tag
 *      daStatic         : DA is static
 *      fPri             : Frame priority bits
 *      qPri             : Queue priority bits
 *      pktCmd           : 0 = Forward packet to destination/s according
 *                       :      to the data below
 *                       : 1 = Drop packet
 *                       : 2 = Trap to CPU
 *                       : 3 = Reserved
 *      srcVlanTagged    : Valid only if <Input_Encapsulation> = Ethernet.
 *                       : 1 = the Ethernet frame has a 802.1Q tag.
 *      srcDevice        : The packet's source device number.
 *      origSrcPortOrTrnk: Packet's original source port or trunk
 *      cascade          : Cascading frame flag
 *      marvellTagCmd    : 0 = Frame directed to CPU port
 *                         1 = Frame arrived from CPU and sent to target port
 *                         2 = Frame should be sent to target sniffer
 *                         3 = Frame forwarded like normal frame
 *      rxSnif           : Rx sniffed frame flag
 *      txSnif           : Tx sniffer frame
 *      pauseTime        :
 *      tagVal           : Frame's tag value
 *      filtered         : Forward/discard frame
 *      xdrop            :
 *      tpv              : Trunk ports vector
 *      mcfi             : CFI bit in Marvell Tag
 *      tagOut           : Frame is tagged om output
 *      srcTrunk         : Source port is trunk member
 *      trunkId          : Trunk ID of the source port.
 *      useDt            : Double TAG using
 *      dtPri            : Double TAG priority
 *      dtVid            : Double TAG VLAN ID
 *      modified         : Frame data was changed
 *      arp              : Frame is ARP
 *      fcIn             : Flow control frame
 *      fcSpd            : Egress port's speed
 *      atuEntry         : ATU entry to store
 *      atuEntryAddr      : address to store ATU entry
 *      data for Opal Plus
 *      priOnlyTag       ;
 *      pppOE            ; pppOE policy mapping control
 *      vBas             ; vBas policy mapping control
 *      eType            ; eType policy mapping control
 *      policyVid        ; policy Vid mapping
 *      policyDa         ; policy SA mapping
 *      policySa         ; policy DA mapping
 *      iHl              ; IP Header Version
 *      priOnlyTag       ; priority tag assign
 *      sBit             ; S bit value
 *      overLimit        ; over limit address
 *      saNoDpv          ; DPV all zeros
 *      uDPpolicy        ; Port Policy uDPpolicy
 *      oPt82policy      ; Port Policy oPt82policy
 *      vBaspolicy       ; Port Policy vBaspolicy
 *      pPppolicy        ; Port Policy pPppolicy
 *      eTypepolicy      ; Port Policy eTypepolicy
 *      vTupolicy        ; Port Policy vTupolicy
 *      sApolicy         ; Port Policy sApolicy
 *      dAPpolicy        ; Port Policy dAPpolicy
 *      polMirror        ; Policy mirror indication
 *      polTrap          ; Policy trap indication
 *      pvt              ; port vlan table map
 *      rmtMngmt         ; remote management indication
 *      reqFormat        ; 3.1 family code 0 foe whole devide 0x0001 for SOHO
 *      reqCode          ; supported request by opal plus Init,Dump ATU,MIB Read/Write Reg
 *      reqData          ; data needed by specific code
 */
typedef struct {
    SBUF_BUF_ID                 frameBuf;
    GT_U16                      byteCount;
    GT_U8        *              dstMacPtr;
    GT_U8        *              ipHeaderPtr;
    GT_U32                      srcPort;
    GT_U16                      vid;
    struct {
        struct {
            GT_U8               useDaPriority;
            GT_U8               daPriority;
        }da_pri;
        struct {
            GT_U8               useVtuPriority;
            GT_U8               vtuPriority;
        }vtu_pri;
        struct {
            GT_U8               useSaPriority;
            GT_U8               saPriority;
        }sa_pri;
    } priorityInfo;
    struct{
        GT_U8                   useIpvxPriority;
        GT_U8                   ipPriority;
        GT_U8                   ieeePiority;
    } ipPriority;
    struct{
        GT_U8                   mld;
        GT_U8                   igmp;
        GT_U8                   pause;
    } daType;
    GT_U8                       igmpSnoop;
    GT_U32                      destPortVector;
    GT_U8                       notRateLimit;
    GT_U32                      dbNum;
    GT_U8                       saHit;
    GT_U8                       saUpdate;
    GT_U8                       daHit;
    GT_U8                       vtuHit;
    GT_U8                       vtuMiss;
    GT_U8                       spOk;
    GT_U32                      vtuVector;
    GT_U8                       Mgmt;
    GT_U8                       daStatic;
    GT_U8                       fPri;
    GT_U8                       qPri;
    GT_U8                       pktCmd;
    GT_U8                       srcVlanTagged;
    GT_U8                       srcDevice;
    GT_U8                       cascade;
    TAG_COMMAND_E               marvellTagCmd;
    GT_U32                      origSrcPortOrTrnk;
    GT_U8                       rxSnif;
    GT_U8                       txSnif;
    GT_U8                       pauseTime;
    /* Rubi specific fields */
    GT_U16                      tagVal;
    GT_U8                       filtered;
    GT_U8                       xdrop;
    GT_U32                      tpv;
    GT_U8                       mcfi;
    GT_U8                       tagOut;
    GT_U8                       srcTrunk;
    GT_U8                       trunkId;
    GT_U8                       useDt;
    GT_U8                       dtPri;
    GT_U8                       dtVid;
    GT_U8                       modified;
    GT_U8                       arp;
    GT_U8                       fcIn;
    GT_U8                       fcSpd;
    /* data to store in the ATU - learning */
    GT_U32                      atuEntry[2];
    GT_U32                      atuEntryAddr;
    /* data for Opal Plus */
    GT_U8                       priOnlyTag;
    GT_U8                       pppOE;
    GT_U8                       vBas;
    GT_U8                       eType;
    GT_U8                       iHl;
    GT_U32                      sBit;
    GT_U32                      policyVid;
    GT_U32                      policyDa;
    GT_U32                      policySa;
    GT_U8                       overLimit;
    GT_U8                       saNoDpv;
    GT_U32                      uDPpolicy;
    GT_U32                      oPt82policy;
    GT_U32                      vBaspolicy;
    GT_U32                      pPppolicy;
    GT_U32                      eTypepolicy;
    GT_U32                      vTupolicy;
    GT_U32                      sApolicy;
    GT_U32                      dAPpolicy;
    GT_U8                       polMirror;
    GT_U8                       polTrap;
    GT_U32                      pvt;
    GT_U8                       rmtMngmt;
    GT_U16                      remethrtype;
    GT_U16                      reqFormat;
    GT_U16                      reqCode;
    GT_U8                       reqData[490];

}SKERNEL_FRAME_SOHO_DESCR_STC;

/*
 * Typedef: struct SKERNEL_FRAME_DESCR_UNT
 *
 * Description:
 *      Describe a descriptors union
 * Fields:
 *      presteraDscr     : Prestera specific descriptor
 *      sohoDscr         : Soho specific descriptor
*/
typedef union {
    SKERNEL_FRAME_DESCR_STC         presteraDscr;
    SKERNEL_FRAME_SOHO_DESCR_STC    sohoDscr;
}SKERNEL_FRAME_DESCR_UNT;


/*******************************************************************************
*   smainFrame2PortSend
*
* DESCRIPTION:
*       Send frame to a correspondent SLAN of a port.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       portNum     - number of port to send frame.
*       dataPrt     - pointer to start of frame.
*       dataSize    - number of bytes to be send.
*       doLoopback  - loopback on portNum for the cheetah device.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smainFrame2PortSend
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN GT_U32                  portNum,
    IN GT_U8    *              dataPrt,
    IN GT_U32                  dataSize,
    IN GT_BOOL                 doLoopback
);

/*******************************************************************************
*   smainFrame2PortSendWithSrcPortGroup
*
* DESCRIPTION:
*       Send frame to a correspondent SLAN of a port. (from context of src port group)
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*       portNum        - number of port to send frame.
*       dataPrt        - pointer to start of frame.
*       dataSize       - number of bytes to be send.
*       doLoopback     - loopback on portNum for the cheetah device.
*       srcPortGroup   - the src port group that in it's context we do packet send
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*        if doLoopback is enabled , then the packet is sent to the portNum
*
*******************************************************************************/
void smainFrame2PortSendWithSrcPortGroup
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  portNum,
    IN GT_U8    *              dataPrt,
    IN GT_U32                  dataSize  ,
    IN GT_BOOL                 doLoopback,
    IN GT_U32                  srcPortGroup
);

/*******************************************************************************
*   smainFrame2CpuSend
*
* DESCRIPTION:
*       Send frame to a CPU by DMA.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       descrPrt    - pointer to descriptor of frame.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smainFrame2CpuSend
(
    IN SKERNEL_DEVICE_OBJECT      *     deviceObj,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt
);



/*******************************************************************************
* smainIsVisualDisabled
*
* DESCRIPTION:
*        Returns the status of the visual asic.
* INPUTS:
*        None
*
* RETURNS:
*       0   - Enabled
*       1   - Disabled
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 smainIsVisualDisabled
(   void
);

/*******************************************************************************
*   smainUpdateFrameFcs
*
* DESCRIPTION:
*       Update FCS value of ethernet frame
*
* INPUTS:
*        deviceObj  - pointer to device object.
*        frameData  - pointer to the data
*        frameSize  - data size
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smainUpdateFrameFcs
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * frameData,
    IN GT_U32  frameSize
);


/* Extended Packet commands */
typedef enum {
    SKERNEL_EXT_PKT_CMD_FORWARD_E = 0,
    SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E,
    SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E,
    SKERNEL_EXT_PKT_CMD_HARD_DROP_E,
    SKERNEL_EXT_PKT_CMD_SOFT_DROP_E,

    SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E,  /* came with DSA tag 'to sniffer' */
    SKERNEL_EXT_PKT_CMD_FROM_CSCD_TO_CPU_E,/* came with DSA tag 'to CPU' */
    SKERNEL_EXT_PKT_CMD_FROM_CPU_E,        /* came with DSA tag 'from CPU' */

    SKERNEL_EXT_PKT_CMD_LAST_E
}SKERNEL_EXT_PACKET_CMD_ENT;

/* Extended Packet commands */
typedef enum {
    SKERNEL_MIRROR_MODE_HOP_BY_HOP_E = 0,
    SKERNEL_MIRROR_MODE_SRC_BASED_OVERRIDE_SRC_TRG_E = 1,
    SKERNEL_MIRROR_MODE_END_TO_END_E = 2,
}SKERNEL_MIRROR_MODE_ENT;

/* Marvell tag command */
typedef enum {
    SKERNEL_MTAG_CMD_TO_CPU_E = 0,
    SKERNEL_MTAG_CMD_FROM_CPU_E,
    SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E,
    SKERNEL_MTAG_CMD_FORWARD_E
}SKERNEL_MTAG_CMD_ENT;


/* Egress classification of packets:
   SKERNEL_EGR_PACKET_CNTRL_UCAST_E -         Control unicast packets
   SKERNEL_EGR_PACKET_CNTRL_MCAST_E -         Control Multicast packets
   SKERNEL_EGR_PACKET_BRG_UCAST_E   -         Bridged unicast packets
   SKERNEL_EGR_PACKET_BRG_REG_MCAST_BCAST_E - Bridged Registered Multicast
                                              or Broadcast
  SKERNEL_EGR_PACKET_BRG_UNK_UCAST_E        - Bridged Unknown unicast packets
  SKERNEL_EGR_PACKET_BRG_UNREG_MCAST_E      - Bridged Unregistered multicast
                                               packets
  SKERNEL_EGR_PACKET_ROUT_UCAST_E           - Unicast Routed packets relayed
  SKERNEL_EGR_PACKET_ROUT_MCAST_E           - Multicast Routed packets relayed
 */
typedef enum {
    SKERNEL_EGR_PACKET_CNTRL_UCAST_E,
    SKERNEL_EGR_PACKET_CNTRL_MCAST_E,
    SKERNEL_EGR_PACKET_BRG_UCAST_E,
    SKERNEL_EGR_PACKET_BRG_REG_MCAST_BCAST_E,
    SKERNEL_EGR_PACKET_BRG_UNK_UCAST_E,
    SKERNEL_EGR_PACKET_BRG_UNREG_MCAST_E,
    SKERNEL_EGR_PACKET_ROUT_UCAST_E,
    SKERNEL_EGR_PACKET_ROUT_MCAST_E
}SKERNEL_EGR_PACKET_TYPE_ENT;

typedef enum {
    SKERNEL_QOS_PROF_PRECED_SOFT = 0,
    SKERNEL_QOS_PROF_PRECED_HARD,
}SKERNEL_QOS_PROF_PRECED_ENT;

typedef enum {
    SKERNEL_PRECEDENCE_ORDER_SOFT = SKERNEL_QOS_PROF_PRECED_SOFT,
    SKERNEL_PRECEDENCE_ORDER_HARD = SKERNEL_QOS_PROF_PRECED_HARD
}SKERNEL_PRECEDENCE_ORDER_ENT;

/*   SKERNEL_FRAME_WIRELESS_BINDING_IDENTIFIER_ENT
 *      The wireless binding identifier.
 *        The following values are defined:
 *        1 -  IEEE 802.11
 *        2 -  IEEE 802.16
 *        3 -  EPCGlobal
*/
typedef enum {
    SKERNEL_FRAME_WIRELESS_BINDING_IDENTIFIER_IEEE_802_11_E = 1,/* according to draft-ietf-capwap */
    SKERNEL_FRAME_WIRELESS_BINDING_IDENTIFIER_IEEE_802_16_E = 2,/* according to draft-ietf-capwap */
    SKERNEL_FRAME_WIRELESS_BINDING_IDENTIFIER_EPCGLOBAL_E   = 3 /* according to draft-ietf-capwap */
}SKERNEL_FRAME_WIRELESS_BINDING_IDENTIFIER_ENT;

/*
    SKERNEL_POLICER_TRIGGER_MODE_ENT : policer trigger mode.

    SKERNEL_POLICER_TRIGGER_MODE_NONE_E - policer was not triggered
    SKERNEL_POLICER_TRIGGER_MODE_PORT_E - 'port mode' - policer triggered by port mode
    SKERNEL_POLICER_TRIGGER_MODE_FLOW_E - 'flow mode' - policer triggered by flow mode
    SKERNEL_POLICER_TRIGGER_MODE_E_PORT_E - 'ePort mode' - policer triggered by ePort mode
    SKERNEL_POLICER_TRIGGER_MODE_E_VLAN_E - 'ePort mode' - policer triggered by ePort mode
*/
typedef enum {
    SKERNEL_POLICER_TRIGGER_MODE_NONE_E,
    SKERNEL_POLICER_TRIGGER_MODE_PORT_E,
    SKERNEL_POLICER_TRIGGER_MODE_FLOW_E,
    SKERNEL_POLICER_TRIGGER_MODE_E_PORT_E,
    SKERNEL_POLICER_TRIGGER_MODE_E_VLAN_E,
}SKERNEL_POLICER_TRIGGER_MODE_ENT;

/*
 *  Structure : SKERNEL_VIRT_PORT_STC
 *
 *  Description : generic virtual port
 *      virtPort - virtual port number
 *      virtDevNum - virtual device number
 *
*/
typedef struct{
    GT_U8   virtPort;
    GT_U8   virtDevNum;
}SKERNEL_VIRT_PORT_STC;

 /*
 *  Structure : SKERNEL_FRAME_CHEETAH_DESCR_802_11_STC
 *
 *  Description : 802.11 info needed encapsulated on the CAPWAP payload
 *
 *       frameControlType    -
 *       frameControlSubType -
 *       frameControlToDs    -
 *       frameControlFromDs  -
 *       frameControlWep - Indicate that the frame body is encrypted
 *           Source Address (SA)  Address of the MAC that originated the frame
 *               Can be on wireless or wired 802 network
 *           Destination Address (DA)  Address of the final recipient(s)
 *               Can be on wireless or wired 802 network
 *           BSS Identifier (BSSID)  Unique identifier of a particular BSS
 *               BSS  Basic Service Set Receiver must examine
 *               BSSID to filter out frames from different BSS
 *               BSSID is important for broadcast/multicast traffic
 *
 *       Mapping of address types to address fields depends on ToDS/FromDS
 *           Address 1 contains address of the immediate WLAN receiver
 *           Address 2 contains address of the immediate WLAN sender
 *           Address 3 usually contains address of the final source or destination
 *               Can be on Wireless LAN or Ethernet LAN
 *           Address 4 is only used for Wireless Bridge  WDS
 *               WDS  Wireless distribution system
 *                       toDs,FromDs
 *                         0,0      0,1     1,0     1,1
 *       address1Ptr -     DA       DA      BSSID   RA
 *       address2Ptr -     SA       BSSID   SA      TA
 *       address3Ptr -   BSSID      SA      DA      DA
 *       address4Ptr -     N/A      N/A     N/A     SA
 *
 *       frameBodyPtr - frame body pointer - after the MAC header of the 802.11
 *                      starting the 802.2 LLC header.
 *       payloadPtr   - payload data starts after the etherType field
 *
 *       bssidPtr -  If 802.11 Frame Control ToDS=1 FromDS=0, this is extracted from Address1
 *                   If 802.11 Frame Control ToDS=1 FromDS=1 (WDS), this is extracted from Address2.
 *       macSaPtr -  If 802.11 Frame Control ToDS=1 FromDS=0, this is extracted from Address2
 *                   If 802.11 Frame Control ToDS=1 FromDS=1 (WDS), this is extracted from Address4
 *       macDaPtr -  If 802.11 Frame Control ToDS=1 FromDS=0, this is extracted from Address3
 *                   If 802.11 Frame Control ToDS=1 FromDS=1 (WDS), this is extracted from Address3
 *       etherType - Extracted from 802.11 LLC header SNAP Type
 *       hasTid - has TID ? tid exist if Frame Control Type = 0b10 (data) and SubType=0b1XXX (QoS)
 *       tid802dot11e - Extracted from 802.11 QoS Control  -- 4 bits
 *                      NOTE: Relevant only if Frame Control Type = 0b10 (data) and SubType=0b1XXX (QoS)
 *
 */

typedef struct{
    GT_U32      frameControlType;
    GT_U32      frameControlSubType;
    GT_BIT      frameControlToDs;
    GT_BIT      frameControlFromDs;
    GT_BIT      frameControlWep;
    GT_U8       *address1Ptr;
    GT_U8       *address2Ptr;
    GT_U8       *address3Ptr;
    GT_U8       *address4Ptr;

    GT_U8       *frameBodyPtr;
    GT_U8       *payloadPtr;

    GT_U8       *bssidPtr;
    GT_U8       *macSaPtr;
    GT_U8       *macDaPtr;
    GT_U32      etherType;
    GT_BOOL     hasTid;
    GT_U32      tid802dot11e;

}SKERNEL_FRAME_CHEETAH_CAPWAP_DESCR_802_11_STC;

typedef struct{
    GT_U8       *macDa;
    GT_U8       *payloadPtr;/* after Ether-type */
}SKERNEL_FRAME_CHEETAH_CAPWAP_DESCR_802_3_STC;


/*
 *   Struct : SKERNEL_FRAME_CHEETAH_CAPWAP_ACTION_STC
 *
 *   description : The CAPWAP action table is accessed from TTI and
 *                 The line index of the matching rule is used
 *                 to index the PCL (PCL - not TTI) action table and extract the
 *                 action to perform.
 *
 *   fields:
 *      tunnelTerminate - Set to indicate the packet is to be tunnel-terminated.
 *       passengerPacketType - Set to 802.11 / 802.3
 *       vlanAssignmentCmd -
 *               802.11 - Set to:
 *                Enable MAC SA based VLAN assignment.
 *                Set VLAN according to this entry VLAN-ID.
 *               802.3 -  For traffic received on the AC-AC tunnel, this should
 *                        be set to:
 *                        VLAN assignment based on 802.3 tag -
 *                           if untagged, VLAN assignment based on this entry VLAN-ID
 *       vlanId - The VLAN assignment.
 *              802.11 - NOTE: This is the default BSSID VLAN assignment.
 *                   It may be overridden by the MAC SA based VLAN assignment.
 *               802.3 - The VLAN assignment for untagged packets.
 *       enableRssiUpdate -
 *              802.11 - If set, the user (MAC SA) RSSI average is updated.
 *              802.3-- NA
 *       enableBridgeAutolearn - Enable/Disable Bridge auto-learning of MAC SA.
 *              802.11 - NOTE: If STAs are statically configured in the FDB,
 *                             auto-learning should be disabled for traffic
 *                             received on the WTP-AC tunnel.
 *       enableNaToCpu - Enable/Disable Bridge sending of New Address Message to CPU
 *              802.11 - NOTE: If STAs are statically configured in the FDB,
 *                       NA2CPU messages should be disabled for traffic received
 *                       on WTP-AC tunnel.
 *       unknownSaCmd - Set to either:
 *                    SOFT DROP
 *                    HARD DROP
 *                    TRAP
 *                    FORWARD
 *                       802.11 - with this entry VLAN-ID Assignment (BSSID based)
 *       qosMode - Set to either:
 *                 Map 802.11e TID to QoSProfile
 *                 Assign QoS Profile using this entry QoS Profile
 *       qosProfile - QoS Profile assignment
 *       up - user priority assignment
 *       qosTidMappingProfile -
 *                   802.11 - Qos Tid Mapping Profile assignment set to a value 0-7.
 *                   802.3-- NA
 *       srcVirtualPortAssignment - This is used for Bridge auto-learning and
 *                   Local Switching.
 *       enableLocalSwitching - This is used to enable/disable unicast and multicast
 *                   local switching on this virtual port.
 *
 *       arpCmd - Set to either:
 *               TRAP to CPU
 *               MIRROR to CPU
 *
 *       ieeeReservedMcastCmdProfile - There are 2 global IEEE Reserved Multicast Command profile.
 *       sstId - source Id for the frames coming from this interface
 *       ingressOsmRedirect - if set the packet is then redirected according to the
 *                     CPU Code table entry for the user-defined CPU code.
*/
typedef struct{
    GT_BIT      tunnelTerminate;
    GT_U32      passengerPacketType;
    GT_U32      vlanAssignmentCmd;
    GT_U16      vlanId;
    GT_BIT      enableRssiUpdate;
    GT_BIT      enableBridgeAutolearn;
    GT_BIT      enableNaToCpu;
    SKERNEL_EXT_PACKET_CMD_ENT  unknownSaCmd;
    GT_U32      qosMode;
    GT_U32      qosProfile;
    GT_U32      up;
    GT_U32      qosTidMappingProfile;
    SKERNEL_VIRT_PORT_STC srcVirtualPortAssignment;
    GT_BIT      enableLocalSwitching;
    SKERNEL_EXT_PACKET_CMD_ENT  arpCmd;
    GT_U32      ieeeReservedMcastCmdProfile;
    GT_U32      sstId;
    GT_BIT      ingressOsmRedirect;

}SKERNEL_FRAME_CHEETAH_CAPWAP_ACTION_STC;

 /*
 *  Structure : SKERNEL_FRAME_CHEETAH_DESCR_CAPWAP_STC
 *
 *  Description : CAPWAP info needed in core descriptor
 *                (CAPWAP - Control and Provisioning of Wireless Access Points)
 *
 *      NOTE : CH3+ is the first device to support it.
 *
 *      Fields:
 *      doLookup - need to do lookup on CAPWAP key
 *      tunnelTerminated - the packet need to be terminated or not.
 *
 *      CAPWAP Control Packet (Discovery Request/Response):
 *      +---------------------------------------------------+
 *      | IP  | UDP | CAPWAP |CAPWAP | Control | Message    |
 *      | Hdr | Hdr | p-amble|Header | Header  | Element(s) |
 *      +---------------------------------------------------+
 *
 *   CAPWAP Control Packet (DTLS Security Required):
 *   +------------------------------------------------------------------+
 *   | IP  | UDP | CAPWAP | DTLS | CAPWAP | Control | Message    | DTLS |
 *   | Hdr | Hdr | p-amble| Hdr  | Header | Header  | Element(s) | Trlr |
 *   +------------------------------------------------------------------+
 *                        \----------- authenticated ------------/
 *                                \------------- encrypted -------------/
 *
 *
 *
 *      CAPWAP Plain Text Data Packet :
 *      +-----------------------------------------+
 *      | IP  | UDP | CAPWAP | CAPWAP | Wireless  |
 *      | Hdr | Hdr | p-amble| Header | Payload   |
 *      +-----------------------------------------+
 *
 *      DTLS Secured CAPWAP Data Packet:
 *      +------------------------------------------------------+
 *      | IP  | UDP | CAPWAP | DTLS | CAPWAP | Wireless | DTLS |
 *      | Hdr | Hdr | p-amble| Hdr  |  Hdr   | Payload  | Trlr |
 *      +------------------------------------------------------+
 *                            \----- authenticated -----/
 *                                  \------- encrypted --------/
 *
 *      preamblePtr - starting with the CAPWAP preamble info .
 *                    All CAPWAP protocol packets are prefixed with the preamble
 *                    header, which is used to identify the frame type that follows
 *                    The CAPWAP preamble header is used to help identify the
 *                    payload type that immediately follows.
 *                    The reason for this header to is avoid needing the perform
 *                    byte comparisons in order to guess whether the frame is
 *                    DTLS encrypted or not
 *      preamblePayloadType - A 4 bit field which specifies the payload type that
 *                    follows the preamble header.
 *                    The following values are supported:
 *                    0 -  Clear text.
 *                       If the packet is received on the data UDP port, the CAPWAP
 *                       stack MUST treat this as a clear text CAPWAP data packet.
 *                       If received on the control UDP port, the CAPWAP stack
 *                       MUST treat this as a clear text CAPWAP control packet.
 *                       If the control packet is not a Discovery Request or
 *                       Response packet, it is illegal and MUST be dropped.
 *                    1 -  DTLS Payload.
 *                       The packet is either a DTLS packet and MAY be a data or
 *                       control packet, based on the UDP port it was
 *
 *
 *      dtlsPtr - The DTLS header provides authentication and encryption
 *                services to the CAPWAP payload it encapsulates.  This protocol is
 *                defined in RFC 4347.
 *                --> when NULL --> no DTLS header
 *      headerPtr - pointer to the CAPWAP header.
 *      word0 - word 0 of the capwap header
 *      radioId:  A 5 bit field which contains the Radio ID number for this
 *                packet.  WTPs with multiple radios but a single MAC Address range
 *                use this field to indicate which radio is associated with the
 *                packet.
 *      headerLen:  A 5 bit field containing the length of the CAPWAP transport
 *                header in 4 byte words (Similar to IP header length).  This length
 *                includes the optional headers.
 *      wirelessBindId:  A 5 bit field which is the wireless binding identifier.  The
 *                 identifier will indicate the type of wireless packet type
 *                 associated with the radio.  The following values are defined:
 *                 1 -  IEEE 802.11
 *                 2 -  IEEE 802.16
 *                 3 -  EPCGlobal
 *
 *          tBit - The Type 'T' bit indicates the format of the frame being
 *                transported in the payload.  When this bit is set to one (1), the
 *                payload has the native frame format indicated by the WBID field.
 *                When this bit is zero (0) the payload is an IEEE 802.3 frame.
 *          fBit: The Fragment 'F' bit indicates whether this packet is a fragment.
 *                When this bit is one (1), the packet is a fragment and MUST be
 *                combined with the other corresponding fragments to reassemble the
 *                complete information exchanged between the WTP and AC.
 *          lBit: The Last 'L' bit is valid only if the 'F' bit is set and indicates
 *                whether the packet contains the last fragment of a fragmented
 *                exchange between WTP and AC.  When this bit is 1, the packet is
 *                the last fragment.  When this bit is 0, the packet is not the last
 *                fragment.
 *          wBit: The Wireless 'W' bit is used to specify whether the optional
 *                wireless specific information field is present in the header.  A
 *                value of one (1) is used to represent the fact that the optional
 *                header is present.
 *          mBit: The M bit is used to indicate that the Radio MAC Address optional
 *                header is present.  This is used to communicate the MAC address of
 *                the receiving radio when the native wireless packet.  This field
 *                MUST NOT be set to one in packets sent by the AC to the WTP.
 *          kBit: The 'Keep-alive' K bit indicates the packet is a data channel
 *                keep-alive packet.  This packet is used to map the data channel to
 *                the control channel for the specified Session ID and to maintain
 *                freshness of the Data Channel.  The K bit MUST NOT be set for data
 *                packets containing user data.
 *      flags:  A set of reserved bits for future flags in the CAPWAP header.
 *              All implementations complying with this protocol MUST set to zero
 *              any bits that are reserved in the version of the protocol
 *              supported by that implementation.  Receivers MUST ignore all bits
 *              not defined for the version of the protocol they support.
 *      fragmentId:  An 16 bit field whose value is assigned to each group
 *              of fragments making up a complete set.  The fragment ID space is
 *              managed individually for every WTP/AC pair.  The value of Fragment
 *              ID is incremented with each new set of fragments.  The Fragment ID
 *              wraps to zero after the maximum value has been used to identify a
 *              set of fragments.
 *      fragmentOffset:  A 13 bit field that indicates where in the payload
 *              will this fragment belong during re-assembly.  This field is valid
 *              when the 'F' bit is set to 1.  The fragment offset is measured in
 *              units of 8 octets (64 bits).  The first fragment has offset zero.
 *              Note the CAPWAP protocol does not allow for overlapping fragments.
 *              For instance, fragment 0 would include offset 0 with a payload
 *              length of 1000, while fragment 1 include offset 900 with a payload
 *              length of 600.
 *
 *  radioMacAddress:  This optional field contains the MAC address of
 *     the radio receiving the packet.  This is useful in packets sent
 *     from the WTP to the AC, when the native wireless frame format is
 *     converted to 802.3 by the WTP.  This field is only present if the
 *     'M' bit is set.  Given the HLEN field assumes 4 byte alignment,
 *     this field MUST be padded with zeroes (0x00) if it is not 4 byte
 *     aligned.
 *
 *     The field contains the basic format:
 *
 *       0                   1                   2                   3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |    Length     |                  MAC Address
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *     radioMacAddressLength:  The number of bytes in the MAC Address field.  The length
 *        field is present since some technologies (e.g., IEEE 802.16)
 *        are now using 64 bit MAC addresses.
 *
 *     radioMacAddressPtr:  The MAC Address of the receiving radio.
 *
 *  wirelessSpecificInformation:  This optional field contains
 *     technology specific information that may be used to carry per
 *     packet wireless information.  This field is only present if the
 *     'W' bit is set.  Given the HLEN field assumes 4 byte alignment,
 *     this field MUST be padded with zeroes (0x00) if it is not 4 byte
 *     aligned.
 *
 *     The field contains the basic format:
 *
 *       0                   1                   2                   3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |  Wireless ID  |    Length     |             Data
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *     wirelessSpecificInformationWirelessId:  The wireless binding identifier.
 *                 The following values are defined:
 *                 1 -  IEEE 802.11
 *                 2 -  IEEE 802.16
 *                 3 -  EPCGlobal
 *
 *     wirelessSpecificInformationLength:  The length of the data field
 *
 *     wirelessSpecificInformationData:  Wireless specific information, defined
 *              by the wireless specific binding.
 *     rssiValid - is field of rssi is valid
 *     rssi -  valid only when 'W' bit is set and using 802.11 . --> when rssiValid = GT_TRUE
 *              taken as 8 MSB of wirelessSpecificInformationData.
 *             -- Received Signal Strength Indicator (RSSI).
 *
 *      payloadHeaderPtr - pointer to the payload header.
 *                              (IEEE 802.11/IEEE 802.16/EPCGlobal)
 *
 *      frame802dot11Info - the payload format info for 802.11
 *
 *      bssidOrTaOrRadioMacPtr - BSSID/TA/Radio MAC address pointer.
 *
 *      validAction - the action was assigned to header info
 *      action - the set of actions to do
 *
 *
 *      descOriginal - original values of fields that modified in the SKERNEL_FRAME_CHEETAH_DESCR_STC
 *
 *
 *      egressInfo - egress Info
 *          daLookUpMatched - does the egress DA MAC lookup had a match
 *          tsOrDa802dot11eEgressEnable - tunnel start indicate to include an 802.11e QoS Header.
 *                                        OR
 *                                        the DA mac lookup matched so traffic destined to this Unicast DA will include an 802.11e QoS Header.
 *          tsOrDa802dot11eEgressMappingTableProfile - Select one of the four 802.1e egress mapping tables
 *                          NOTE: relevant when tsOrDa802dot11eEgressEnable = 1
 *          tid       - tid to be used for egress frame via the tunnel start (field of 802.11e)
 *          ackPolicy - ackPolicy to be used for egress frame via the tunnel start (field of 802.11e)
 *
*/
typedef struct{
    GT_BOOL  doLookup;
    GT_BOOL  tunnelTerminated;

    GT_U8   *preamblePtr;
    GT_U32  preambleVersion;
    GT_U32  preamblePayloadType;
    GT_U8   *dtlsPtr;
    GT_U8   *headerPtr;
    GT_U32  word0;
    /* fields in word 0 */
    GT_U32  version;
    GT_U32  radioId;
    GT_U32  headerLen;
    SKERNEL_FRAME_WIRELESS_BINDING_IDENTIFIER_ENT  wirelessBindId;
    GT_BIT  tBit;
    GT_BIT  fBit;
    GT_BIT  lBit;
    GT_BIT  wBit;
    GT_BIT  mBit;
    GT_BIT  kBit;
    GT_U32  flags;
    /* fields in word 1 */
    GT_U32  fragmentId;
    GT_U32  fragmentOffset;
    /* fields in (optional) Radio MAC Address */
    GT_U32  radioMacAddressLength;
    GT_U8   *radioMacAddressPtr;
    /* fields in (optional) Wireless Specific Information */
    SKERNEL_FRAME_WIRELESS_BINDING_IDENTIFIER_ENT  wirelessSpecificInformationWirelessId;
    GT_U32  wirelessSpecificInformationLength;
    GT_U32  wirelessSpecificInformationData;
    GT_BOOL rssiValid;
    GT_U32  rssi;
    /* the payload header */
    GT_U8   *payloadHeaderPtr;

    SKERNEL_FRAME_CHEETAH_CAPWAP_DESCR_802_11_STC frame802dot11Info;
    SKERNEL_FRAME_CHEETAH_CAPWAP_DESCR_802_3_STC  frame802dot3Info;
    GT_U8   *passengerL3Ptr;/*pointer to the L3 in the passenger of 802.3/802.11 */

    GT_U8   *bssidOrTaOrRadioMacPtr;

    /********************/
    /* the action to do */
    /********************/
    GT_BOOL             validAction;
    SKERNEL_FRAME_CHEETAH_CAPWAP_ACTION_STC action;

    /* original values of fields that modified in the frame descriptor
       SKERNEL_FRAME_CHEETAH_DESCR_STC */
    struct{
        GT_U8                *macSaPtr;
        GT_U8                *macDaPtr;
        GT_BIT               origIsTrunk;
        GT_U32               origSrcPortOrTrnk;
        GT_U32               srcDev;
        GT_U32               up;
        GT_U32               qosProfile;
        GT_U32               vid;
        SKERNEL_MAC_TYPE_ENT macDaType;
        GT_U32               etherTypeOrSsapDsap;
    }descOriginal;

    struct{
        GT_BIT  daLookUpMatched;
        GT_BIT  tsOrDa802dot11eEgressEnable;
        GT_U32  tsOrDa802dot11eEgressMappingTableProfile;
        GT_U32  tid;
        GT_U32  ackPolicy;

        GT_U8   *address1Ptr;
        GT_U8   *address2Ptr;
        GT_U8   *address3Ptr;
        GT_U8   *address4Ptr;

    }egressInfo;

}SKERNEL_FRAME_CHEETAH_DESCR_CAPWAP_STC;

typedef enum{
    SKERNEL_FRAME_TUNNEL_START_TYPE_MPLS_E,/*x-over-mpls*/
    SKERNEL_FRAME_TUNNEL_START_TYPE_IPV4_E,/*X-over-IPv4 or X-Over-IPv4/GRE*/   /* not applicable to Sip5 */
    SKERNEL_FRAME_TUNNEL_START_TYPE_MIM_E,/*MacInMac*/
    /* sip5*/
    SKERNEL_FRAME_TUNNEL_START_TYPE_TRILL_E = 3, /*Ethernet-over-TRILL*/

    /* remove when generic types support will be complete */
    SKERNEL_FRAME_TUNNEL_START_TYPE_CAPWAP_E, /*CAPWAP*/
    /* remove when generic types support will be complete */
    SKERNEL_FRAME_TUNNEL_START_TYPE_IPV6_E, /*IPV6*/

    SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV4_E = 6, /*generic-IPV4*/
    SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV6_E = 7  /*generic-IPV6*/
}SKERNEL_FRAME_TUNNEL_START_TYPE_ENT;


/* PCL mode for accessing the PCL configuration table (for each lookup)*/
typedef enum{
    SKERNEL_PCL_LOOKUP_MODE_PORT_E = 0,                /* port index */
    SKERNEL_PCL_LOOKUP_MODE_VID_E = 1,                /* vid index */
    SKERNEL_PCL_LOOKUP_MODE_EPORT_E = 2,               /* ePort index */
    SKERNEL_PCL_LOOKUP_MODE_DESCRIPTOR_INDEX_E = 0xff  /* descriptor index that may be set by TTI/PCL action index -->
                                                        see descrPtr->ipclProfileIndex */
} SKERNEL_PCL_LOOKUP_MODE_ENT;


/**
 * typedef: enum SKERNEL_FRAME_TR101_VLAN_TAG_CLASS_ENT
 *
 * Description: TAG0 and TAG1 Classification for DSA/Non-DSA packets
 *
 * Enumerations:
 *
 *  SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E      - inner VLAN tag
 *  SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E      - outer VLAN tag
 *  SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E  - Tag Not Found
 *
 * COMMENTS:
 *
 * ******************************************************************************/
typedef enum {
    SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E,
    SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E,
    SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E
}SKERNEL_FRAME_TR101_VLAN_TAG_CLASS_ENT;

/**
 * typedef: enum SKERNEL_FRAME_TR101_VLAN_EGR_TAG_STATE_ENT
 *
 * Description: Per Port Egress VLAN state options
 *
 * Enumerations:
 *
 *  SKERNEL_FRAME_TR101_VLAN_EGR_TAG_UNTAGGED_E  - untagged
 *  SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG0_E      - Tag0
 *  SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG1_E      - Tag1
 *  SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG0_IN_TAG1_E - Outer Tag0, Inner Tag1
 *  SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG1_IN_TAG0_E - Outer Tag1, Inner Tag0
 *  SKERNEL_FRAME_TR101_VLAN_EGR_TAG_PUSH_TAG0_E - Push Tag0
 *  SKERNEL_FRAME_TR101_VLAN_EGR_TAG_POP_OUT_TAG_E - Pop outer Tag
 *  SKERNEL_FRAME_TR101_VLAN_EGR_TAG_DO_NOT_MODIFIED_E - Do not modify any tag, packet tags are sent as received
 *                  supported on eArch device
 * COMMENTS:
 *
 * ******************************************************************************/
typedef enum {
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_UNTAGGED_E,
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG0_E,
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG1_E,
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG0_IN_TAG1_E,
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG1_IN_TAG0_E,
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_PUSH_TAG0_E,
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_POP_OUT_TAG_E,

    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_DO_NOT_MODIFIED_E

}SKERNEL_FRAME_TR101_VLAN_EGR_TAG_STATE_ENT;

/**
 * typedef: enum SKERNEL_FRAME_TR101_VLAN_INGR_TAG_STATE_ENT
 *
 * Description: Per Port Ingress VLAN state options
 *
 * Enumerations:
 *
 *  SKERNEL_FRAME_TR101_VLAN_INGR_TAG_UNTAGGED_E  - untagged
 *  SKERNEL_FRAME_TR101_VLAN_INGR_TAG_TAG0_E      - Tag0
 *  SKERNEL_FRAME_TR101_VLAN_INGR_TAG_TAG1_E      - Tag1
 *  SKERNEL_FRAME_TR101_VLAN_INGR_TAG_OUT_TAG0_IN_TAG1_E - Outer Tag0, Inner Tag1
 *  SKERNEL_FRAME_TR101_VLAN_INGR_TAG_OUT_TAG1_IN_TAG0_E - Outer Tag1, Inner Tag0
 *
 * COMMENTS:
 *
 * ******************************************************************************/
typedef enum {
    SKERNEL_FRAME_TR101_VLAN_INGR_TAG_UNTAGGED_E    = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_UNTAGGED_E,
    SKERNEL_FRAME_TR101_VLAN_INGR_TAG_TAG0_E        = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG0_E,
    SKERNEL_FRAME_TR101_VLAN_INGR_TAG_TAG1_E        = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG1_E,
    SKERNEL_FRAME_TR101_VLAN_INGR_TAG_OUT_TAG0_IN_TAG1_E = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG0_IN_TAG1_E,
    SKERNEL_FRAME_TR101_VLAN_INGR_TAG_OUT_TAG1_IN_TAG0_E = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG1_IN_TAG0_E,
}SKERNEL_FRAME_TR101_VLAN_INGR_TAG_STATE_ENT;

/*check if tag 0 exist */
#define TAG0_EXIST_MAC(_descPtr)    \
            (((_descPtr)->ingressVlanTag0Type == SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E) ?      \
                0 : 1)

/*check if tag 1 exist */
#define TAG1_EXIST_MAC(_descPtr)    \
            (((_descPtr)->ingressVlanTag1Type == SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E) ?      \
                0 : 1)

/*check if packet not tagged or priority tagged */
#define UNTAGGED_OR_PRIORITY_TAGGED_MAC(_srcTagState , _srcPriorityTagged)    \
            (((_srcTagState) == SKERNEL_FRAME_TR101_VLAN_INGR_TAG_UNTAGGED_E ||/*packet with no tags */   \
              _srcPriorityTagged) ?/*packet with priority tag */  \
                1 : 0)

/*check if packet not tagged or priority tagged -- on tag 1 */
#define UNTAGGED_OR_PRIORITY_TAGGED_TAG1_MAC(_descPtr)    \
        (((TAG1_EXIST_MAC(_descPtr) == 0) ||    /* not with tag 1 */     \
          (TAG1_EXIST_MAC(_descPtr) && (_descPtr)->vid1 == 0)) ? 1 : 0) /* with tag 1 but with value 0 */



/* IPM bridging search type */
typedef enum {
    SNET_CHEETAH_IPM_SEARCH_SIP_DIP_E = 0,
    SNET_CHEETAH_IPM_SEARCH_DIP_E
}SNET_CHEETAH_IPM_SEARCH_ENT;


typedef struct {
    GT_BIT              valid; /* 0 - VLAN entry is invalid. 1 - VLAN entry is valid.*/
    GT_BIT             unknownIsNonSecurityEvent; /* 0 - Do not learn MAC Source Addresses for packets classified with this VID,
                                                (update security breach register).
                                                1 - Learn MAC Source Addresses for packets classified with this VID.*/
    GT_BIT             portIsMember; /* 0 - Source Port is not a member of this VLAN.
                                        1 - Source Port is a member of this VLAN.*/
    SKERNEL_STP_ENT    spanState; /*  STP of a port take from STG */
    SKERNEL_EXT_PACKET_CMD_ENT unregNonIpMcastCmd; /*  Filter command for unregistered Multicast packets (does not include MAC Broadcast)
                                                    that are neither IPv4 nor IPv6 Multicast packets (identified by not having a MAC destina-
                                                    tion prefix of either 01-00-5E-xx-xx-xx/25 or 33-33-xx-xx-xx-xx/16 */
    SKERNEL_EXT_PACKET_CMD_ENT unregIPv4McastCmd;  /*Filter command for unregistered IPv4 Multicast packets*/
    SKERNEL_EXT_PACKET_CMD_ENT unregIPv6McastCmd;  /*Filter command for unregistered IPv6 Multicast packets*/
    SKERNEL_EXT_PACKET_CMD_ENT unknownUcastCmd;    /* Filter command for unknown Unicast packets*/
    SKERNEL_EXT_PACKET_CMD_ENT unregNonIp4BcastCmd;     /* Filter command for unregistered non-IPv4 Broadcast packets */
    SKERNEL_EXT_PACKET_CMD_ENT unregIp4BcastCmd;     /* Filter command for unregistered IPv4 Broadcast packets */


    GT_BIT                     igmpTrapEnable; /* 0 - Don't trap igmp packet
                                            1 - Trap igmp packet*/

    GT_BIT                     ipv4IpmBrgEn ; /* If this bit is enable we should lookup the destination address in mac table
                                                according to ipv4 DIP/SIP.*/

    SNET_CHEETAH_IPM_SEARCH_ENT ipv4IpmBrgMode; /* for ipm packet search:
                                                        0 - (SIP,DIP)
                                                        1 - (*.DIP) */

    GT_BIT                     ipv6IpmBrgEn ; /* If this bit is enable we should lookup the destination address in mac table
                                                according to ipv6 DIP/SIP.*/
    SNET_CHEETAH_IPM_SEARCH_ENT ipv6IpmBrgMode;/* for ipm packet search:
                                                        0 - (SIP,DIP)
                                                        1 - (*.DIP) */

    GT_BIT                      ingressMirror ; /* Enable/Disable ingress mirroring of traffic to this VLAN to Ingress Analyzer Port. */
    GT_BIT                      icmpIpv6TrapMirror; /* Enable/Disable ICMPv6 Trapping or Mirroring to the CPU, according to glo-balICMpv6 message type table.*/

    GT_BIT                      ipInterfaceEn; /* If this bit is enable simulation should mirror the packet to cpu
                                                     in case the (Cht2 - only Ipv4) packet is one of the following:
                                                    1. IP Link Local Multicast control
                                                    2. ARP BC
                                                    3. RIPv1
                                                    4. IPv6 Neighbor Solicitation Multicast (only Cht)*/

    GT_BIT                      ipv4UcRoutEn;   /*Enable IPv4 Unicast Routing for this VLAN */

    GT_BIT                      ipv6UcRoutEn;   /*Enable IPv6 Unicast Routing for this VLAN */
    /* Cheetah2 only fields */
    GT_U32                      mruIndex;   /* The MRU index. This VLAN's MRU configuration packets are associated with.
                                               The VLAN with a byte count larger then the configured MRU are assigned a Hard Drop command*/
    GT_BIT                      bcUdpTrapMirrorEn; /* Enable Trap/Mirror of BC udp packet based on their destination udp port */
    GT_BIT                      autoLearnDisable;/* Disable(=1) Automatic learning of specific VLAN entry */
    GT_BIT                      naMsgToCpuEn;    /* Enable(=0) na masg to cpu  */
    GT_BIT                      ipV6InterfaceEn; /* If this bit is enable , simulation should mirror the IPv6 packet to cpu
                                                     in case the packet is one of the following:
                                                    1. IP Link Local Multicast control check
                                                    2. IPv6 Neighbor Solicitation Multicast */
    GT_BIT                      ipV6SiteID;     /* used by the router for ipv6 scope checking */
    GT_BIT                      ipv4McRoutEn;   /*Enable IPv4 Multicast Routing for this VLAN */

    GT_BIT                      ipv6McRoutEn;   /*Enable IPv6 Multicast Routing for this VLAN */

    GT_BIT                      floodVidxMode;   /* Determines whether unknown UC/ MC / BC is flooded
                                                    to the subset of ports pointed by <VIDX>
                                                    0x0 = DontFlood;
                                                          Assign VIDX from the VLAN entry for flooded traffic
                                                          of type Unreg MC only;
                                                          other flooded traffic is assigned VIDX = 0xFFF
                                                     0x1 = Flood;
                                                          Assign VIDX from VLAN entry for ALL flooded traffic
                                                          (I.e. Unreg MC, BC, Unk UC). */
    GT_U32                      floodVidx;       /* Default Vidx (For Bridge over tunnels feature)
                                                    - If the VLAN does not contain any virtual ports this
                                                      entry is configured with a VIDX of 0xFFF (default)
                                                    - If the VLAN contains virtual bridge ports this entry
                                                      is configured with a VIDX in the virtual VIDX range.
                                                      Note: this VIDX table entry should be configured with
                                                      all 1s so it is equivalent to flooding to the VLAN port list.
                                                 */
    GT_BIT                      ucLocalEn;      /* Unicast traffic local switching enable.
                                                   When enabled, AND <UCLocalEn> bit in the corresponding
                                                   ingress port bridge configuration entry is enabled,
                                                   a packet ingressing on the configured ingress port
                                                   that belongs to this VLAN, can be switched back to its
                                                   local source device+port / trunk.
                                                   When either of these bits is disabled, the packet will
                                                   be assigned with a SOFT DROP command.*/
    GT_U32                      vrfId;          /* vrf Id */


    SKERNEL_EXT_PACKET_CMD_ENT  unknownSaCmd;   /* command for unknown SA , do
                                                   resolution with 'per port' command.
                                                   Only relevant in controlled address learning mode,
                                                   when <Auto-learning enable> = Disabled.
                                                   The bridge forwarding decision for packets
                                                   with unknown Source Address */
    GT_U32                      analyzerIndex;/* index of mirror to analyzer */
    GT_U32                      ipv4McBcMirrorToAnalyzerIndex;/*
                                    When enabled, IPv4 MC/BC packets in this eVLAN are mirrored to the
                                    analyzer specified in this field.
                                    This is independent of the analyzer configuration of other traffic in this eVLAN.
                                    0x0 is a reserved value that means Disabled
                                    */
    GT_U32                      ipv6McMirrorToAnalyzerIndex;
                                    /*
                                    When enabled, IPv6 MC packets in this eVLAN are mirrored to the
                                    analyzer.
                                    This is independent of the analyzer configuration of other traffic in this eVLAN.
                                    0x0 is a reserved value that means Disabled
                                    */
    GT_U32                      fid;/*Used for virtual bridges per eVLAN.
                                    Used by the bridge engine for entry lookup and entry match
                                    (replaces VID in bridge entry)*/

    GT_U32                      fdbLookupKeyMode; /*
                                    Used to support a mode where the key is {MAC, FID, VID1}.
                                    When the global FDB IVL mode is enabled, this field determines the FDB lookup key:
                                    0x0 = Single tag FDB lookup. FDB Lookup key is {MAC, FID}
                                    0x1 = Double tag FDB Lookup. FDB Lookup key is {MAC, FID, VID1}
                                    (sip5_10)*/


}SNET_CHEETAH_L2I_VLAN_INFO;

typedef struct {
    GT_BIT mcLocalEn;                           /* Enables sending L2 multicast packets back to the sender.
                                                   This feature is controlled by Vlan (here) and by port configuration in TxQ */
    GT_U32 portIsolationVlanCmd;                /* Controls Port Isolation feature in the TxQ.
                                                    It also has global enabling configuration in the TxQ.
                                                     0x0 = Disable; Port Isolation filter is disabled.
                                                     0x1 = L2PortIsoEn; Trigger L2 Port Isolation filter.
                                                     0x2 = L3PortIsoEn; Trigger L3 Port Isolation filter.
                                                     0x3 = AllPortIsoEn; Trigger L2 & L3 Port Isolation filter
                                                */
}SNET_CHEETAH_EGR_VLAN_INFO;

typedef enum{
    EPCL_ACTION_MODIFY_NO_TAG_E = 0,    /*no modify*/
    EPCL_ACTION_MODIFY_OUTER_TAG_E = 1, /*modify outer tag*/
    EPCL_ACTION_MODIFY_TAG0_E = 2       /*sip5 : modify tag 0 on the egress packet (not necessarily the outer tag)*/
}EPCL_ACTION_MODIFY_TAG_ENT;


typedef enum{
    EPCL_ACTION_MODIFY_NO_DSCP_EXP_E = 0,    /*no modify EXP/DSCP */
    EPCL_ACTION_MODIFY_OUTER_DSCP_EXP_E = 1, /*modify outer EXP/DSCP*/
    EPCL_ACTION_MODIFY_INNER_DSCP_E = 2 /*sip5 : modify inner DSCP , but not support inner EXP*/
}EPCL_ACTION_MODIFY_DSCP_EXP_ENT;


/* common actions for after HA changes : EPLR,EPCL */
typedef struct {
    GT_U32          drop;   /* drop/forward */

    EPCL_ACTION_MODIFY_TAG_ENT          modifyUp;/* modify UP
                                0 - no modify
                                1 - modify outer tag
                                2 - modify tag 0 on the egress packet (not necessarily the outer tag)
                                    mode 2 supported from lion3
                            */
    GT_U32          up;
    EPCL_ACTION_MODIFY_DSCP_EXP_ENT          modifyDscp; /* modify DSCP */
    GT_U32          dscp;/* DSCP */
    EPCL_ACTION_MODIFY_TAG_ENT          modifyVid; /* modify VID - not relevant to EPLR
                                0 - no modify
                                1 - modify outer tag
                                2 - modify tag 0 on the egress packet (not necessarily the outer tag)
                                    mode 2 supported from lion3
                              */
    GT_U32          vid;
    EPCL_ACTION_MODIFY_DSCP_EXP_ENT          modifyExp; /* modify EXP */
    GT_U32          exp;/* EXP */

    GT_BIT          modifyTc; /* modify TC */
    GT_U32          tc;
    GT_BIT          modifyDp; /* modify DP */
    GT_U32          dp;

/*    -- fields relevant from lion3 --- */

    GT_U32          vlan1Cmd;   /*tag 1 vlan command*/
    GT_U32          vid1;       /*vlan to set into tag 1 according to  vlan1Cmd*/
    GT_U32          modifyUp1;  /*Enable Modify UP1*/
    GT_U32          up1;        /*UP to set into tag 1 according to  modifyUp1*/



}SNET_CHEETAH_AFTER_HA_COMMON_ACTION_STC;

/**
 * Typedef: struct SNET_LION_GTS_ENTRY_STC
 *
 * Description:
 *      Describe a timestamp entry format.
 *
 * Fields:
 *
 *      ptpVersion  -   1 or 2 (for v1 or v2)
 *      seqID       -   SequenceID field from the PTP packet header.
 *      msgType     -   MessageType field from the PTP packet header.
 *      transportSpecific
 *                  -   TransportSpecific field from the PTP header.
 *      srcTrgPort  -   For ingress timestamping this fields specifies the local source port,
 *                      while for egress timestamping this field specifies the local target port.
 *      V2DomainNumber
 *                  -   domain number from PTPv2 header
 *      V1Subdomain - subdomain from PTPv1 header (128 bits).
 *
 * Comments:
 */
typedef struct {
    GT_U32 ptpVersion;
    GT_U32 seqID;
    GT_U32 msgType;
    GT_U32 transportSpecific;
    GT_U32 srcTrgPort;
    GT_U32 V2DomainNumber;
    GT_U32 V1Subdomain[4];
}SNET_LION_GTS_ENTRY_STC;

/**
 * Typedef: struct SNET_LION_PTP_GTS_INFO_STC
 *
 * Description:
 *      Timestamp info structure.
 *
 * Fields:
 *
 *      ptpPacketTriggered  - packet identified as PTP and triggered by ingress/egress PTP mechanism
 *      ptpMessageHeaderPtr - (pointer to) the start of the PTP header
 *      gtsEntry            - timestamp entry format
 *
 * Comments:
 */
typedef struct {
    GT_BIT    ptpPacketTriggered;
    GT_U8     *ptpMessageHeaderPtr;
    SNET_LION_GTS_ENTRY_STC gtsEntry;
}SNET_LION_PTP_GTS_INFO_STC;

typedef struct {
    GT_U32 freezeCounter;
    GT_U32 prevPckts[2];
    GT_U64 prevBytesCnt;
}SNET_XCAT_IPFIX_ERRATA_STC;

/* relevant to eArch device only */
/* isTrgPhyPortValid - Cleared by all mechanisms that assign ePort*/
#define SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(dev,_descPtr,unitName)    \
    _descPtr->eArchExtInfo.isTrgPhyPortValid = 0

typedef enum{
    SNET_CHT_FRAME_PARSE_MODE_PORT_E,
    SNET_CHT_FRAME_PARSE_MODE_TRILL_E,
    SNET_CHT_FRAME_PARSE_MODE_FROM_TTI_PASSENGER_E,

    SNET_CHT_FRAME_PARSE_MODE_LAST_E    /* must be last . needed for array size */
}SNET_CHT_FRAME_PARSE_MODE_ENT;


/* HA internal info */
typedef struct{
    GT_BIT   tunnelStart;/* indication to do tunnel start*/
    GT_BIT   tunnelStartPassengerType;   /* Type of passenger packet being encapsulated.*/
    GT_BIT   doRouterHa;    /* DA modification indication */
    GT_U32   arpPointer;    /* ARP Pointer relevant when doRouterHa == 1 */
    GT_BIT   routed;        /* SA modification indication */
    GT_BIT   retainCrc;     /*Relevant when the passenger packet is Ethernet.
                        Determines whether the Ethernet passenger packet 4-bytes of CRC should be retained.
                        NOTE: When the passenger packet CRC field is retained in the packet it is unmodified,
                              even if the passenger packet was changed by the device.
                              Therefore, when this field is set to 1 and the passenger packet is modified,
                              the retained CRC transmitted in the packet does not reflect the changes made
                              to the passenger packet.
                              0x0 = Remove;
                                    The Ethernet passenger packet 4-bytes of CRC are removed and the tunneled
                                    packet is transmitted with a newly generated CRC for the entire packet.
                              0x1 = Don't Remove; The Ethernet passenger packet 4-bytes of CRC are not removed,
                                    nor modified, and the tunneled packet is transmitted with two CRC fields,
                                    the passenger packet original, unmodified CRC field and the newly generated
                                    CRC for the entire packet.*/
    GT_U8   *tsIpv4HeaderPtr;/* pointer to the ipv4 header of the tunnel start, used when need to calculate
                                <total length> and <checksum> when the passenger is Ethernet */
    GT_U8   *tsIpv6HeaderPtr;/* pointer to the ipv6 header of the tunnel start, used when need to
                                calculate <payload length> when the passenger is Ethernet */
    GT_U8   *tsUdpHeaderPtr;/* pointer to the UDP header of the tunnel start, used when need to
                                calculate <length> when the passenger is Ethernet */

    struct{
        GT_BIT  vlanTagged;     /* is the TS need vlan tag */
        GT_U16  vlanId;         /* the vlan tag of the tunnel (regardless to vlan tag of ETH passenger) */
        GT_U8   cfi;            /* the CFI of the tunnel (regardless to CFI of ETH passenger) */
        GT_U8   up;             /* the UP of the tunnel (regardless to UP of ETH passenger) */
        GT_U32  vlanEtherType;  /* the ethertype of the tunnel (regardless to ethertype of ETH passenger) */
    }tsVlanTagInfo;

    GT_U32  tunnelAssigningMode;/*SIP5_10 : Selecting between the Tunnel DSCP / UP - selecting between the Passenger values and the QosMapped values
            (As part of the fix for JIRA <HA-3283> and JIRA <HA-3340>)
            0=Passenger:Assign DSCP / UP to the tunnel according to the passenger fields
            1=QosMappedValues:Assign DSCP / UP according to the Qos Mapped values
            */

    /* EVB/BPE info */
    GT_U8    *evbBpeTagPtr;    /* pointer to EVB/BPE tags */
    GT_U32   evbBpeTagLength;  /* length of EVB/BPE tag (0/4/8/6) */
    GT_BIT   evbBpeIsPassenger;/* is the EVB/BPE apply on L2 of TS or of passenger
                               0 - the EVB/BPE on TS header (in case of TS)
                               1 - the EVB/BPE on Passenger (in case of TS)
                               if non TS --> both behave the same.*/

}HA_INTERNAL_INFO_STC;

typedef enum{
    SHT_PACKET_CLASS_1_E,
    SHT_PACKET_CLASS_2_E,
    SHT_PACKET_CLASS_3_E
}SHT_PACKET_CLASS_ENT;

enum{
    SNET_CHT_TAG_0_INDEX_CNS = 0,/* 'tag 0' index */
    SNET_CHT_TAG_1_INDEX_CNS , /* 'tag 1' index */

    SNET_CHT_TAG_MAX_INDEX_CNS/* MAX VALUE */
};


/**
 * Typedef: struct SKERNEL_E_ARCH_EXT_INFO_STC
 *
 * Description:
 *      extra eArch info in the descriptor.
 *
 * Fields:
 *
 * Comments:
 */
typedef struct{
    GT_U32          vidx;   /* 12 bits vidx for the TXQ use .
                               Setting by EQ.
                               Used by TxQ,HA.

                               Relevant when useVidx == 1
                               */

    GT_U32          trgPhyPort; /* 8 bits target port for the TXQ use .
                                Target physical port (not necessarily in the current device).
                                Passed throughout the pipe, and in DSA.
                                Setting by EQ.
                                Used by TxQ, HA.
                                Change: Extend to 8 bits

                                Relevant when useVidx == 0
                                */
    GT_BIT          isTrgPhyPortValid;/*This field is conveyed throughout the pipe,
                                    and through the DSA tag.
                                    Set by TTI (based on DSA), EQ E2PHY.
                                    Cleared by all mechanisms that assign ePort.
                                    Used by E2PHY trigger logic, HA for setting DSA Tag

                                Relevant when useVidx == 0
                                */

    GT_U32          defaultSrcEPort;/* default eport value : for DUBUG USE ONLY !!! */

    GT_U32          localDevSrcEPort;/*
                            Setting by TTI.
                            Used by TTI, IPCL, L2I, Router, EQ, TxQ, HA.
                            This field used in ePort enabled units.
                            Assigned default Port<ePort> and may be overridden by
                            TTI Src ePort assignment mechanisms

                            20 bits field
                            */
    struct{
        GT_BIT          sniffUseVidx;/* do we use sniffEVidx*/
        GT_U32          sniffEVidx; /* eVidx for sniff . 16 bits */
        GT_U32          sniffTrgEPort;/* ePort for sniff . 20 bits */
        GT_U32          sniffVidx;/* vidx for sniff . 12 bits (from EQ--> TXQ)*/
        GT_BIT          sniffisTrgPhyPortValid;/* is target port valid */
    }toTargetSniffInfo; /* Target Analyzer Info when the descriptor is ToTargetSniffer */

    GT_U32          srcTrgEPort;/* src/target ePort ,
                        when : If <OutGoingMtagCmd>==TO_TARGET_SNIFFER || <OutGoingMtagCmd>==TO_CPU

                            20 bits field
                        */


    GT_U32          *ttiPhysicalPortAttributePtr;/* pointer to the entry in : tti - Physical Port Attribute Table  */
    GT_U32          *ttiPhysicalPort2AttributePtr;/* pointer to the entry in : tti - Physical Port 2 Attribute Table  */

    GT_U32          *ttiPreTtiLookupIngressEPortTablePtr;/*pointer to the entry in : tti - Pre-TTI Lookup Ingress ePort Table
                                the purpose of saving this pointer is to allow:
                                1. quick access to it.
                                2. limit the errors due to changes in the field of descrPtr->eArchExtInfo.localDevSrcEPort*/

    GT_U32          *ttiPostTtiLookupIngressEPortTablePtr;/* pointer to the entry in : tti - Post-TTI Lookup Ingress ePort Table  */

    GT_U32          *bridgeIngressEPortTablePtr;/*pointer to the entry in : Bridge Ingress ePort Table
                                the purpose of saving this pointer is to allow:
                                1. quick access to it.
                                2. limit the errors due to next logic of access to the table:
                                    If (InDesc<PortIsRingCorePort> = 1)
                                        index = InDesc<OrigSRCePort/Trunk>
                                    else
                                        index = InDesc<LocaDevSRCePort>
                                */
    GT_U32          *ipvxIngressEPortTablePtr;/*pointer to the entry in : IPvX  Ingress ePort Table*/
    GT_U32          *ipvxIngressEVlanTablePtr;/*pointer to the entry in : IPvX  Ingress eValn Table*/
    GT_U32          *eqIngressEPortTablePtr;/*pointer to the entry in : EQ  Ingress ePort Table*/
    GT_U32          *txqEgressEPortTablePtr;/*pointer to the entry in : TXQ Egress  ePort Table*/
    GT_U32          *egfShtEgressEPortTablePtr; /*pointer to the entry in : EGF-SHT Egress  ePort Table*/
    SHT_PACKET_CLASS_ENT egfShtEgressClass; /* classification for EGF-SHT */
    GT_U32          *egfQagEgressEPortTablePtr; /*pointer to the entry in : EGF-QAG Egress  ePort Table*/
    GT_U32           egfQagEgressEPortTable_index;/* index used to access egfQagEgressEPortTable */

    GT_U32          *haEgressEPortAtt1TablePtr; /*pointer to the entry in : HA  Egress  ePort Att 1 Table*/
    GT_U32           haEgressEPortAtt1Table_index;/* index used to access haEgressEPortAtt1Table */
    GT_U32          *haEgressEPortAtt2TablePtr; /*pointer to the entry in : HA  Egress  ePort Att 2 Table*/
    GT_U32           haEgressEPortAtt2Table_index;/* index used to access haEgressEPortAtt2Table */

    GT_U32          *haEgressPhyPort1TablePtr; /*pointer to the entry in : HA  Physical port Attributes 1 Table*/
    GT_U32           haEgressPhyPort1Table_index;
    GT_U32          *haEgressPhyPort2TablePtr; /*pointer to the entry in : HA  Physical port Attributes 2 Table*/
    GT_U32           haEgressPhyPort2Table_index;

    GT_U32          *haEgressVlanTablePtr; /*pointer to the entry in : HA  Egress eVlan Table*/

    struct{
        GT_U32          eqIngressEPort;/*the ePort that used in EQ for accessing "EQ  Ingress ePort Table"*/
        GT_BIT          IN_descTrgIsTrunk;/* hold the value of the 'IN descriptor' to the EQ , 'target is trunk'  */
        GT_BIT          IN_descTrgEPort;/* hold the value of the 'IN descriptor' to the EQ , 'target EPort'  */
        GT_BIT          IN_descDrop;/* used for all devices (not only eArch) hold the indication if the 'IN descriptor' to the EQ , was in 'soft/hard' drop  */
    }eqInfo;/* EQ extra info */


    struct{
        GT_BIT      excludedIsPhyPort;/*Used to determine if the <Excluded ePort> field contains an ePort or a physical port */
    }fromCpu;

   GT_BIT          assignTrgEPortAttributesLocally; /* Setting by TxQ (dev map table).
                            Used by HA, EPCL */

    struct{
        GT_BIT      tunnelStart;                /* is tunnel start needed ?
                                                   NOTE: when this flag set to 1 the "tunnel Start Engine"
                                                        logic need to use desc->haInfo.tunnelStartPointer and not
                                                        desc->tunnelPtr !!! */
        GT_U32      tunnelStartPointer;         /* tunnel start index
                                                    relevant when desc->haInfo.tunnelStart = 1*/
        GT_BIT      ePortModifyMacSa;           /* the egress ePort triggers  modify the mac SA */
        GT_BIT      ePortModifyMacDa;           /* the egress ePort triggers  modify the mac DA */

        GT_U32      arpPointer;                 /* Arp pointer - used for CNC counting */


        GT_BIT      doubleTagToCpu; /*force_new_dsa_to_cpu*/
        GT_BIT      forceNewDsaFwdFromCpu;/*force_new_dsa_forward_or_from_cpu*/
        GT_BIT      rxTrappedOrMonitored;/* from ingress pipe trap to cpu or to analyzer --- 'need original info' */
        GT_BIT      dsaReplacesVlan;/* dsa instead of outer vlan tag (or in addition or no outer vlan anyway) */
        GT_BIT      stripL2;/* strip L2 mac in the passenger or routed */
        GT_U32      tpidIndex_atStartOfHa[SNET_CHT_TAG_MAX_INDEX_CNS];/* tpid index for TPID0,TPID1 - as saved at the start of HA unit */

        struct{
            GT_BIT  dsa_tag0_Src_Tagged;
            GT_BIT  dsa_tag0_is_outer_tag;
            GT_BIT  dsa_tag1_Src_Tagged;
            GT_U32  dsa_tpid_index;
        }dsa_forword;

    }haInfo;/* header alteration extra into*/

    GT_BIT packetIsTT;/*An indication to the CPU that the trapped/mirrored packet was tunnel-terminated by the device
                        processing pipes. When set, according to a global configuration, the passenger packet offset
                        may be sent in this eDSA tag instead of <Flow-ID / TT Offset>
                        0 = No TT: The packet was not tunnel-terminated by the device processing engines
                        1 = TT: The packet was tunnel-terminated by the device processing engines. According to a
                        global configuration in the device, the passenger packet offset in the tunnel is sent in the
                        <Flow-ID / TT Offset> field
                        NOTE: This field is relevant only for ingress analyzed (SrcTrg == SRC), where the original
                        packet was marked for TT.*/


    GT_BIT          origSrcPhyIsTrunk;/*Indicates whether the packet was received in the ingress device on a physical port that is a
                            trunk member
                            0 = Packet arrived on physical port that is configured as non-trunk member
                            1 = Packet arrived on physical port that is configured as a trunk member*/

    GT_U32          origSrcPhyPortTrunk;/*Indicates the source physical ingress port/trunk number in the ingress devices, according to
                            the value of <OrigSrcPhyIsTrunk>*/

    GT_BIT          phySrcMcFilterEn;/* Indicates to the remote device whether to perform VIDX MC source filtering based on <OrigSrcPhyPort/Trunk>
            0 = Do not perform VIDX physical source filtering
            1 = Perform VIDX physical source filtering */

}SKERNEL_E_ARCH_EXT_INFO_STC;

typedef struct{
    GT_U8   *macDaSaPtr;/* pointer to actual mac addresses on the egress packet (for TS this point to macDa of ethernet passenger if exists)
                        so can be NULL when TS with passenger that is not ethernet !!! */
    GT_U8   *outerVlanTagPtr;/* pointer to actual outer vlan tag on the egress packet (NULL when not tagged)
                                in case of tunnel start this is the tunnel tag and not the passenger tag */
    GT_U8   *timeStampTagPtr; /* pointer to timestamp tag on the egress packet (NULL when not tagged) */
    GT_U8   *vlanTag0Ptr;/* pointer to vlan tag 0 on the egress packet (NULL when not tagged)
                                in case of tunnel start of ETH this is the passenger tag 0 */
    GT_U8   *vlanTag1Ptr;/* pointer to vlan tag 1 on the egress packet (NULL when not tagged)
                                in case of tunnel start of ETH this is the passenger tag 1 */

    GT_U8   *l3StartOffsetPtr; /* pointer to l3 header start (after the 2 bytes of 'ethertype')*/

    GT_U8   *tunnelStartL3StartOffsetPtr; /* if TS then this pointer to l3 header start of TS and NOT in passenger (after the 2 bytes of 'ethertype') */
    GT_U8   *tunnelStartL2StartOffsetPtr; /* if TS then this pointer to l2 header start of TS and NOT in passenger */

    /* used by HA after packet finished build */
    GT_BIT  xponChanges; /* do we have xpon vid changes */
    GT_U32  xPonVid;/*GPON vlan id*/

    GT_BIT  modifyVid;/* do we have vid changes - due to 'Vlan translation' */
    GT_U32  outerVid;/*outer vlan id - due to 'Vlan translation'*/

    GT_U32 prePendLength;/* the number of bytes for pre-pend (value 0 or 2) */

    GT_U32 evbBpeRspanTagSize;/* number of bytes that the EVB/BPE/RSPAN added */

    /* info used by EPCL key and by the 'final egress remark' */
    struct{
        GT_U32  qosMapTableIndex;/* index 0..11 of the 12 qos tables */
        GT_BIT  egressTcDpMapEn;
        GT_BIT  egressUpMapEn;
        GT_BIT  egressDscpMapEn;
        GT_BIT  egressExpMapEn;
        GT_BIT  egressDpToCfiMapEn;
    }qos;

    GT_BOOL useArpForDa;/* indication that the arp pointer was used for mac DA */
    GT_BOOL useArpForSa;/* indication that the arp pointer was used for mac SA */
    GT_U32  paddingZeroForLess64BytesLength;/* the padding that we do to egress at least 64 bytes */

    GT_U32  epclKeyVid;/* set and used by EPCL : valid from SIP5 : the VID that the EPCL should use for build key (instead of eVid) */
    struct{
        GT_BIT  tag1_exists;
        GT_BIT  tag1_is_outer;
    }bpe_802_1br_ingressInfo;

}SKERNEL_HA_TO_EPCL_INFO_STC;

typedef struct {
    GT_U32 oamProfile;      /* Setting by TTI/IPCL/EPCL when the IPCL Action<OAM Processing Enable> is set. Used by Ingress OAM unit. */
    GT_U32 opCode;          /* Setting by IPCL/EPCL when the IPCL Action<OAM Processing Enable> is set. Used by Ingress OAM unit. */
    GT_U32 megLevel;        /* Setting by IPCL/EPCL when the IPCL Action<OAM Processing Enable> is set. Used by Ingress OAM unit. */
    GT_BIT oamProcessEnable;/* Setting by TTI/IPCL/EPCL when the IPCL Action<OAM Processing Enable> is set. Used by Ingress OAM unit. */
    GT_BIT oamEgressProcessEnable;/* Setting by EPCL when the EPCL Action<OAM Processing Enable> is set. Used by Egress OAM unit. */
    GT_BIT lmCounterCaptureEnable; /* Setting by the Ingress/Egress OAM unit. Used by the IPLR */
    GT_BIT lmCounterInsertEnable;  /* Setting by IPCL/EPCL to select between LM counter and Timestamp insertion. used by PSU. */
    GT_U32 lmCounter;       /* Setting by the EPLR. Used by the Packet Stamping Unit. */
    GT_BIT timeStampEnable; /* Setting by TTI/IPCL/Ingress OAM/Egress OAM unit. Used by Packet Stamping Unit. */
    GT_U32 timeStampTagged; /* Setting by TTI, according to header parsing. Used by: HA (PSU). */
    GT_U32 offsetIndex;     /* Setting by TTI/IPCL/Ingress OAM/Egress OAM unit. Used by Packet Stamping Unit for inserting the LM counter or DM timestamp */
    GT_U64 hashIndex;       /* Setting by TTI, used by egress OAM unit. */
    GT_U32 oamTxPeriod;     /* Setting by IPCL/EPCL. Used by IOAM/EOAM. */
    GT_U32 oamRdi;          /* Setting by IPCL/EPCL. Used by IOAM/EOAM. */
}SKERNEL_OAM_INFO_STC;

typedef enum{
    SKERNEL_INNER_PACKET_TYPE_IP                = 0,
    SKERNEL_INNER_PACKET_TYPE_MPLS              = 1,
    SKERNEL_INNER_PACKET_TYPE_ETHERNET_WITH_CRC = 2,
    SKERNEL_INNER_PACKET_TYPE_ETHERNET          = 3,
    SKERNEL_INNER_PACKET_TYPE_UNKNOWN           = 4
}SKERNEL_INNER_PACKET_TYPE_ENT;

typedef enum{
    SKERNEL_XCAT_TTI_MPLS_NOP_E      = 0,
    SKERNEL_XCAT_TTI_MPLS_SWAP_E     = 1,
    SKERNEL_XCAT_TTI_MPLS_PUSH_E     = 2,
    SKERNEL_XCAT_TTI_MPLS_POP_1_E    = 3,
    SKERNEL_XCAT_TTI_MPLS_POP_2_E    = 4,
    SKERNEL_XCAT_TTI_MPLS_POP_3_E    = 5,       /* new in sip5 */
    SKERNEL_XCAT_TTI_MPLS_POP_SWAP_E = 7,

    /* SIP5 combination of commands with 'SWAP' */
    SKERNEL_XCAT_TTI_MPLS_POP2_SWAP_E = 8,
    SKERNEL_XCAT_TTI_MPLS_POP3_SWAP_E = 9


}SKERNEL_XCAT_TTI_MPLS_COMMAND_ENT;

typedef enum{
    SKERNEL_TIMESTAMP_TAG_TYPE_UNTAGGED_E       = 0,
    SKERNEL_TIMESTAMP_TAG_TYPE_NON_EXTENDED_E   = 1,
    SKERNEL_TIMESTAMP_TAG_TYPE_EXTENDED_E       = 2,
    SKERNEL_TIMESTAMP_TAG_TYPE_HYBRID_E         = 3,
    SKERNEL_TIMESTAMP_TAG_TYPE_PIGGY_BACKED_E   = 4
}SKERNEL_TIMESTAMP_TAG_TYPE_ENT;

typedef enum{
    SKERNEL_TIMESTAMP_TAG_MODE_NONE_E               = 0,
    SKERNEL_TIMESTAMP_TAG_MODE_ALL_NON_EXTENDED_E   = 1,
    SKERNEL_TIMESTAMP_TAG_MODE_ALL_EXTENDED_E       = 2,
    SKERNEL_TIMESTAMP_TAG_MODE_NON_EXTENDED_E       = 3,
    SKERNEL_TIMESTAMP_TAG_MODE_EXTENDED_E           = 4,
    SKERNEL_TIMESTAMP_TAG_MODE_PIGGYBACK_E          = 5,
    SKERNEL_TIMESTAMP_TAG_MODE_HYBRID_E             = 6
}SKERNEL_TIMESTAMP_TAG_MODE_ENT;

typedef enum{
    SKERNEL_PTP_TRIGGER_TYPE_PTP_OVER_L2_E          = 0,
    SKERNEL_PTP_TRIGGER_TYPE_PTP_OVER_IPV4_UDP_E    = 1,
    SKERNEL_PTP_TRIGGER_TYPE_PTP_OVER_IPV6_UDP_E    = 2,
    SKERNEL_PTP_TRIGGER_TYPE_RESERVED_E             = 3
}SKERNEL_PTP_TRIGGER_TYPE_ENT;

typedef struct{
    SKERNEL_TIMESTAMP_ACTION_ENT          action;
    SKERNEL_TIMESTAMP_PACKET_FORMAT_ENT   packetFormat;
    SKERNEL_PTP_TRIGGER_TYPE_ENT          transportType;
    GT_U32                                offset;
    GT_U32                                ptpMessageType;
    GT_U32                                ptpDomain;
}SNET_TIMESTAMP_ACTION_INFO_STC;

typedef struct{
    GT_BIT    portProtMatch;        /* port protocol match */
    GT_U32*   portProtMatchedMemoryPointer;/* when port protocol match : pointer to the memory of the entry*/
}SNET_PER_PROTOCOL_INFO_STC;

#define SKERNEL_FRAME_CHEETAH_TRILL_OPTIONS_MAX_PARSE_BYTES_CNS  40

typedef struct{
    GT_U32  trillCpuCodeBase;/*cpu code base incase of exception command 'to_cpu'/'mirror' */


    GT_U32  trillMcDescriptorInstance;/*
                All multi-target TRILL packets received are always duplicated by Descriptor Duplication engine.
                If ALL the following conditions are TRUE, the TRILL multi-target packet is duplicated:
                 1. EtherType == Global<TRILL EtherType>
                 2. TRILL.M == 1
                 3. desc<Cmd> == FORWARD/MIRROR
                Each descriptor instance is subject to TTI Lookup.
                The TTI TRILL Key distinguishes between the two instances using the key field:
                <TRILL MC Descriptor Instance>
                The 1st desc instance has key field assignment <TRILL MC Desc Instance> = 0
                The 2nd desc instance has key field assignment <TRILL MC Desc Instance> = 1
                */
    /*
    TRILL Header immediately follows the TRILL Ethertype:
                                    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                    | V | R |M|Op-Length| Hop Count |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | E-RBID/TREE-ID | I-RBID |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    GT_U32  V;/*V (Version): 2-bits*/
    GT_BIT  M;/*M (Multi-destination): 1-bit*/
    GT_U32  opLength;/*Op-Length (Options Length): 5-bit*/
    GT_U32  hopCount;/*Hop Count: 6-bit*/
    GT_U32  eRbid;/*E-RBID: 16-bit
        -When M==0, this is the Egress RBridge
        -When M==1, this is the TREE-ID
        */
    GT_U32  iRbid;/*I-RBID: 16-bit Ingress RBridge*/

    /*
    If TRILL.Op-Length is > 0, the TRILL header is immediately followed by 4*Op-Length bytes of option
    data. TTI must parse the 2 MS bits of the first word of the option data.
    +------+------+----+----+----+----+----+----+
    | CHbH | CItE | Reserved                    |
    +------+------+----+----+----+----+----+----+
    */
    GT_BIT  CHbH;/*CHbH - Critical hop-by-hop Options are present.
                    -If set, packet options must be processed by all RBridges*/
    GT_BIT  CItE;/*CItE  Critical Ingress to Egress Options are present
                    -If set, packet options must be processed by the Egress RBridge or packet must be dropped*/

}SNET_TRILL_INFO_STC;

typedef struct{ /* valid info during egress to specific physical port */
    struct SKERNEL_DEVICE_OBJECT_T *txDmaDevObjPtr;/* pointer to the 'TX DMA port' egress device object . */
    GT_U32                          txDmaMacPort;/* local port number of the 'TX DMA port'
                                       TX MAC number:
                                       Extended mode:  port 9 - MAC 12, port 11 - MAC 14
                                       Normal mode: MAC number = port number
                               */
    GT_U32                          localPortIndex;/* local port that converted from global physical port */
    GT_U32                          globalPortIndex;/* global physical port */

    GT_U32                          egressPipeId;/* the pipe id for the egress port */
    GT_U32                          egressDpIndexId;/* the DP index that handle the egress port (LOCAL within the 'pipe')*/
}SNET_EGRESS_PHYSICAL_PORT_INFO_STC;

typedef struct{ /* ingressDsa */ /* filled in snetChtMarvellTagParsing */
    GT_BIT    origIsTrunk;
    GT_U32    origSrcEPortOrTrnk;
    GT_U32    srcDev;
    GT_U32    qosProfile;
    GT_U32    srcId;

    GT_U32    dsaWords[SKERNEL_EXT_DSA_TAG_4_WORDS_E+1];/*the 4 words of the DSA , for shorter DSA , pad with 0 */
    GT_BIT    fromCpu_egressFilterEn;/*the FROM_CPU field of egressFilterEn */

}SNET_INGRESS_DSA_STC;

typedef enum{
    SNET_INGRESS_TUNNEL_TRANSIT_TYPE__NONE__E,/* not transit */

    SNET_INGRESS_TUNNEL_TRANSIT_TYPE_LEGACY_IP_OVER_MPLS_TUNNEL_E,
    /*next are sip5 only*/
    SNET_INGRESS_TUNNEL_TRANSIT_TYPE_X_OVER_MPLS_E,
    SNET_INGRESS_TUNNEL_TRANSIT_TYPE_X_OVER_NON_MPLS_E
}SNET_INGRESS_TUNNEL_TRANSIT_TYPE_ENT;

typedef struct{
    SNET_INGRESS_TUNNEL_TRANSIT_TYPE_ENT    transitType;
    /* info for IPCL */
    struct SKERNEL_FRAME_CHEETAH_DESCR_STCT  * origDescrPtr;/* the original descriptor */
    struct SKERNEL_FRAME_CHEETAH_DESCR_STCT  * passengerDescrPtr;/* pointer to the transit descriptor */
    struct SKERNEL_FRAME_CHEETAH_DESCR_STCT  * fixedFieldsTypeDescrPtr;/* pointer to the descriptor for fixed fields */
    struct SKERNEL_FRAME_CHEETAH_DESCR_STCT  * udbDescrPtr;/* pointer to the descriptor for UDB anchors  */

    struct SKERNEL_FRAME_CHEETAH_DESCR_STCT* innerFrameDescrPtr;/*
                                        in the TTI,IPCL units :
                                        when parsing TRILL packets there is need
                                        to use values from the 'inner frame'
                                        as well as values from the 'outer frame'.
                                         inner packet used for :
                                         1. QoS Trust Mode
                                         2. IPCL key fields
                                         3. Packet hash key

                                        and this is regardless to 'Tunnel termination' !
                                        so the 'Inner descriptor' will hold the 'Inner frame'

                                        pointer type is SKERNEL_FRAME_CHEETAH_DESCR_STC */

    GT_BIT  innerPacketL2FieldsAreValid;/*Inner packet L2 fields are valid.
        The TTI is capable of parsing up to a maximum option length of 40B.
        If the options are greater than this the payload will not be parsed.

        when  innerPacketL2FieldsAreValid == 0 the next fields are not valid
            innerMacDaPtr,innerTag0Exists,innerPacketTag0Vid
         */

    GT_U8   *innerMacDaPtr;/* inner mac DA pointer */
    /* next relevant to the Inner packet (passenger) of MIM and TRILL packets */
    GT_BIT  innerTag0Exists;/* Inner packet (passenger) outer tag exists */
    GT_U32  innerPacketTag0Vid;/*Inner packet (passenger) outer tag VID*/
    GT_U32  innerPacketTag0CfiDei;/*Inner packet (passenger) outer tag cfi/dei*/
    GT_U32  innerPacketTag0Up;/*Inner packet (passenger) outer tag UP */

}SNET_INGRESS_TUNNEL_INFO_STC;/* relevant for MIM,TRILL,MPLS tunnels */

typedef struct {
    /*not relevant to 'from cpu ' */
     GT_U32                qosProfile;   /* QoS Profile index for not FROM_CPU frame, set by frame parser, PCL, L2i or policer */
     GT_BIT                ingressExtendedMode;   /* sip5 eDSA tag only, ingress QoS extended mode: relevant from TTI to EPLR */
     GT_BIT                egressExtendedMode;    /* sip5 eDSA tag only,  egress QoS extended mode: relevant from HA to ERMRK */

    /*only to 'from cpu ' */
     struct{
        GT_BIT               contolTc;   /* Control TC for Packet when it is forwarded over the stack for FROM_CPU frame, set by frame parser, If Extended <ControlTC> <= Mtag<ControlTC, else <ControlTC> <= 1 */
        GT_U8                fromCpuTc;   /* From CPU packet traffic class. for FROM_CPU frame, set by frame parser, If Extended <FromCPU TC[2:0]> <= Mtag<TC[2:0], else <FromCPU TC[2:0]> <= {1'b0,Mtag<TC[1:0]>} */
        GT_U8                fromCpuDp;   /* From CPU packet DP. for FROM_CPU frame, set by frame parser, If Extended <FromCPU DP[1:0]> <= Mtag<DP[1:0],else <FromCPU DP[1:0]> <= 2'b0 */
     } fromCpuQos;
}SNET_QOS_INFO_STC;

/* bmp of flags : ses below */
typedef GT_U32  SKERNEL_MIRROR_TYPE_BMP;
#define SKERNEL_MIRROR_TYPE_STC_CNS         (1<<0)
#define SKERNEL_MIRROR_TYPE_MIRROR_CNS      (1<<1)

/* take field of 'num_of_tags_to_pop' from action table or from src eport table and convert it to number of bytes */
#define SNET_CONVERT_POP_TAGS_FEILD_TO_BYTES_MAC(dev,field) \
    (((field) == 1) ? 4 :     \
     ((field) == 2) ? 8 :     \
     (SMEM_CHT_IS_SIP5_GET(dev) && (field) == 3) ? 6 : 0)



/* macro SKERNEL_PORTS_BMP_IS_PORT_SET_MAC
    to check if port is set the bmp of ports (is corresponding bit is 1)

  portsBmpPtr - of type SKERNEL_PORTS_BMP_STC*
                pointer to the ports bmp
  portNum - the port num to set in the ports bmp

  return 0 -- port not set in bmp
  return 1 -- port set in bmp
*/
#define SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(portsBmpPtr, portNum)   \
    (((portsBmpPtr)->ports[(portNum) >> 5] & (1 << ((portNum) & 0x1f)))? 1 : 0)

/* macro SKERNEL_PORTS_BMP_ADD_PORT_MAC
    to add the port to the bmp of ports

  portsBmpPtr - of type SKERNEL_PORTS_BMP_STC*
                pointer to the ports bmp
  portNum - the port num to set in the ports bmp

*/
#define SKERNEL_PORTS_BMP_ADD_PORT_MAC(portsBmpPtr, portNum)   \
    (portsBmpPtr)->ports[(portNum) >> 5] |= (1 << ((portNum) & 0x1f))

/* macro SKERNEL_PORTS_BMP_DEL_PORT_MAC
    to delete the port from the bmp of ports

  portsBmpPtr - of type SKERNEL_PORTS_BMP_STC*
                pointer to the ports bmp
  portNum - the port num to set in the ports bmp

*/
#define SKERNEL_PORTS_BMP_DEL_PORT_MAC(portsBmpPtr, portNum)   \
    (portsBmpPtr)->ports[(portNum) >> 5] &= (~ (1 << ((portNum) & 0x1f)))

/* macro SKERNEL_PORTS_BMP_CLEAR_MAC
    to clear the bmp of ports

  portsBmpPtr - of type SKERNEL_PORTS_BMP_STC*
                pointer to the ports bmp

*/
#define SKERNEL_PORTS_BMP_CLEAR_MAC(portsBmpPtr)                        \
    SKERNEL_PORTS_BMP_MEMSET_MAC(portsBmpPtr,0)

/* macro SKERNEL_PORTS_BMP_MEMSET_MAC
    to set all the words with the same value in the bmp of ports

  portsBmpPtr - of type SKERNEL_PORTS_BMP_STC*
                pointer to the ports bmp
  value      - the value to set to all the words

*/
#define SKERNEL_PORTS_BMP_MEMSET_MAC(portsBmpPtr,value)                 \
    {                                                                   \
        GT_U32 _ii;                                                     \
                                                                        \
        for(_ii = 0 ;                                                   \
            _ii < SKERNEL_CHEETAH_EGRESS_PORTS_BMP_NUM_WORDS_CNS;       \
            _ii++)                                                      \
        {                                                               \
            (portsBmpPtr)->ports[_ii] = value;                          \
        }                                                               \
    }


#define  SKERNEL_CHEETAH_EGRESS_PORTS_BMP_NUM_WORDS_CNS        ((SKERNEL_CHEETAH_EGRESS_MAX_PORT_CNS + 31) / 32)

/* macro SKERNEL_PORTS_BMP_IS_EMPTY_MAC
    to check if the bmp is 'ALL relevant words' value 0
  portsBmpPtr - of type SKERNEL_PORTS_BMP_STC*
                pointer to the ports bmp

  return 0 -- not all relevant words are 0
  return 1 -- all relevant words are 0
*/
#define SKERNEL_PORTS_BMP_IS_EMPTY_MAC(dev,portsBmpPtr)   \
    snetChtEgressPortsBmpIsEmpty(dev,portsBmpPtr)

/*  Egress Pipe : build bmp of ports from the pointer to the memory (registers/table entry).
    Fill the SKERNEL_PORTS_BMP_STC a for all devices - for Lion B0 and above
    a copy 2 words from memory into SKERNEL_PORTS_BMP_STC,
    for others a copy word 0 into SKERNEL_PORTS_BMP_STC, and word[1] = word[0] & (1 << 31).
    This means that CPU port is copied to word 1 too */
#define SKERNEL_FILL_PORTS_BITMAP_MAC(dev, bmp, mem)     \
    snetChtEgressGetPortsBmpFromMem((dev),(mem),(bmp))


/*  BMP operators :
    operator1 - operator on bmp 1
    operator2 - operator on bmp 2
    resultBmp - the bmp for the result of the operators.

*/
#define SKERNEL_PORTS_BITMAP_OPERATORS_MAC(dev, operator1, bmp1Ptr , operator2, bmp2Ptr, resultBmpPtr)     \
    snetChtEgressPortsBmpOperators(dev, operator1, bmp1Ptr , operator2, bmp2Ptr, resultBmpPtr)

    /*
 * Typedef: structure SKERNEL_PORTS_BMP_STC
 *
 * Description: Defines the bmp of ports (up to 63 ports)
 *
 * Fields:
 *      ports - array of bmp of ports
 *              ports[0] - hold bmp of the ports 0  - 31
 *              ports[1] - hold bmp of the ports 32 - 63
 *                  ...
 *              ports[7] - hold bmp of the ports ..255
 *
 *  notes:
 *  can use macro:
 *  SKERNEL_PORTS_BMP_IS_PORT_SET_MAC
 *
 */
typedef struct SKERNEL_PORTS_BMP_STCT{
    GT_U32      ports[SKERNEL_CHEETAH_EGRESS_PORTS_BMP_NUM_WORDS_CNS];
}SKERNEL_PORTS_BMP_STC;

typedef struct SKERNEL_FRAME_CHEETAH_DESCR_STCT
{
    GT_U32                      frameId;  /* global frame id */

    SBUF_BUF_ID                 frameBuf;
    GT_U8                     * startFramePtr; /* pointer to the first offset of frame*/
    GT_U32                      byteCount;     /* frame size, in cut-through mode configuration which defines the value of desc<BC> associated to Cut Through packet. Should be ignored by all byte-based features */
    GT_U32                      origByteCount; /* length of the hole packet in bytes */
    GT_U32                      localDevSrcPort;/* source port number in the Local Device, Cpu port = 63,
                                                    keep the existing semantics, that is, local device source PHYSICAL port
                                                */
    GT_U32                      localDevSrcPort_fromRxDma_forBma;/* sip5 only : 'localDevSrcPort' as set by the 'rxDma' --> not changed in the pipe !
                                                        check with BMA configuration to find BM/BMA miss-configurations */
    GT_U32                      ingressGopPortNumber;/* the port number for accessing the GOP registers. not this number represent ALL GOP accessing.
                                                in Lion2,3 the 'mac port number' can be different then the 'rxdma port number'
                                                */
    GT_U32                      ingressRxDmaPortNumber;/* the port number for accessing the RXDMA registers.
                                                in sip5 ingressRxDmaPortNumber can be != localDevSrcPort
                                                 due to:
                                                  in single core device (bc2) : ' remote physical port'
                                                  in multi core device (lion3): the localDevSrcPort is 'global port' while ingressRxDmaPortNumber is local port.
                                                                                (and the ' remote physical port')
                                                */
    /* Set by Rx Port */
    GT_BIT                      marvellTagged; /* 1 - frame came with Marvell tag, Set by Rx Port */
    SKERNEL_MTAG_CMD_ENT        incomingMtagCmd;/* the DSA tag command , relevant when marvellTagged == 1*/
    GT_U32                      ownDev;        /* own device id of device */
    SKERNEL_MAC_TYPE_ENT         macDaType; /*MAC DA Type: set by Rx Port and Frame Parser */
    GT_U8                      * macSaPtr; /* Pointer to the MAC SA */
    GT_U8                      * macDaPtr; /* Pointer to the MAC DA */

    GT_U8                     *payloadPtr; /* pointer to the start of payload
                                            in case of Tunnel termination set to start of passenger.
                                                       if TT of CAPWAP set to point after the L2 ethertype.(L3 header)
                                            otherwise set to start of L3 header
                                         */
    GT_U32                     payloadLength;/* length of frame from start of payload */

    GT_U8*                     origVlanTagPtr;/* pointer to the start of vlan tag --
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse
                                                 */
    GT_U32                     origVlanTagLength;/* length of frame from start of vlan tag till start of ethertype
                                                include up to 4 nested vlans
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse
                                                */
    /* */
    GT_BIT                     origSrcTagged;/* 1 - frame came with vlan tag (according to one of the 2 ingress etherTypes) -- or not
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse */
    /*GT_BIT                     srcTagged; --> replaced with tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] */    /* 1 - source frame was VLAN tagged, set by Frame Parser /pcl/tti */
    SKERNEL_FRAME_TR101_VLAN_INGR_TAG_STATE_ENT srcTagState;  /* Per Port Ingress VLAN state options (Tag0 VLAN and Tag1 VLAN classification)
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse */
    SKERNEL_FRAME_TR101_VLAN_INGR_TAG_STATE_ENT origSrcTagState;  /* Per Port Ingress VLAN state options (Tag0 VLAN and Tag1 VLAN classification)
                                                 set only in first parse and NOT from tti re-parse */
    GT_BIT                     srcPriorityTagged;    /* 1 - source frame was VLAN tagged,with VID = 0 set by Frame Parser */
    GT_BIT                     nestedVlanAccessPort;    /* 1 - the ingress port is nested VLAN access port */
    SKERNEL_EXT_DSA_TAG_TYPE_ENT marvellTaggedExtended;
                                /* 0 - frame came with 1 word of DSA.
                                   1 - frame came with 2 words of DSA.
                                   2 - frame came with 3 words of DSA.  -- not supported format
                                   3 - frame came with 4 words DSA tag .

                                   Used by PCL, L2I, EQ, rdDMA, HA
                                   */
    DSA_TAG_TYPE_E             egrMarvellTagType;  /* type of egress DSA tag (none,standard,extended) */
    GT_U32    egress_dsaWords[SKERNEL_EXT_DSA_TAG_4_WORDS_E+1];/*Egress DSA : the 4 words of the DSA , for shorter DSA , pad with 0 */

    /* the info is relevant for packets that did tunnel termination but still need info from the 'tunnel header' */
    struct{
        GT_U32      origMplsOuterLabel;/* the MPLS most outer label */

        GT_U8       *originalL2Ptr; /* pointer to the Original start of L2 in case of T.T .
                                     save value during first packet parse , not modified during TTI re-parse*/
        GT_U8       *originalL3Ptr; /* pointer to the Original start of L3 in case of T.T .
                                     save value during first packet parse , not modified during TTI re-parse*/
        GT_U16      origEtherType; /* original etherType in case of T.T .
                                     save value during first packet parse , not modified during TTI re-parse*/
       SKERNEL_L2_ENCAPSULATION_TYPE_ENT    l2Encaps;/* original l2Encaps in case of T.T .
                                     save value during first packet parse , not modified during TTI re-parse*/

        GT_BIT  arp;
        GT_BIT  mpls;
        GT_BIT isIp;
        GT_BIT isIPv4;
    }origInfoBeforeTunnelTermination;

    GT_U8                     * l3StartOffsetPtr; /* pointer to the first offset of L3 header,set by Frame Parser
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse*/
    GT_U32                      l2HeaderSize;/* the size of L2 header : I.E l3StartOffsetPtr-startFramePtr ,
                                                set only on the first header parsing, not modified after TTI re-parse */
    GT_U8                     * l4StartOffsetPtr; /* pointer to the first offset of L4 header,set by Frame Parser
                                                 set in snetChtL3L4ProtParsing --> modified from first parse and tti re-parse*/
    GT_U32                      l23HeaderSize;/* the size of L2+L3 header : I.E l4StartOffsetPtr-startFramePtr
                                                 set in snetChtL3L4ProtParsing --> modified from first parse and tti re-parse*/
    GT_U8                      *afterVlanOrDsaTagPtr;/*pointer after the vlan tag/dsa tag
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse */
    GT_U32                     afterVlanOrDsaTagLen;/* length until the L3 info
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse */
    GT_BIT                     l2Valid; /* is the L2 info valid -- can be not valid when passenger is not ethernet */
    GT_BIT                     l4Valid; /* is the L4 info valid -  If packet is IPv4/IPv6 with a Valid L4 header,
                                           set to 1, else set to 0 Note: If packet is TT, Take from Inner packet */
    GT_BIT                     l3NotValid;/* indication that L3 is not valid */
    GT_BIT                     modifyUp;         /* 1 - modify UP(VPT) in the egress, set by PCL, L2i, Policer, EQ, TXQ*/
    GT_U32                     up;         /* 1 - UP(VPT) , set by Frame Parser, EQ, */
    GT_U32                     qosMappedUp;/* sip5_10 : the UP that was in the QOS profile entry in the EQ. used by HA for TunnelStart
                                            this is fix for HA-3340 "Incorrect assignment of tunnel's UP" */
    GT_U32                     qosMappedDscp;/* sip5_10 : the DSCP that was in the QOS profile entry in the EQ.
                                                NOTE: in the HW the EQ unit desc->dscp always sets it according to "DSCP that was in the QOS profile entry"
                                                and the use of 'modifyDscp' is done in HA and not in EQ.
                                                but simulation behave differently so we need 'qosMappedDscp' to also have the "DSCP that was in the QOS profile entry"
                                                */
    GT_BIT                     modifyDscp;      /* 1 - modify DSCP in the egress, set by PCL, L2i, Policer, EQ, TXQ*/

    SNET_QOS_INFO_STC           qos;

   GT_BIT                       egressTagged;/* packet egress from port tagged/untagged --
                                                set per egress sending by header alteration .
                                                used by the EPCL */
   GT_BIT                       trgTagged; /*  packet is to be sent via the egress port tagged. Set by Frame Parser, EQ. TXQ*/
   GT_BIT                       preserveVid; /* if set 1 not change VID*/
   GT_BIT                       vidModified; /*  packet's VID was modified. Set by Frame Parser, PCL, TXQ*/
   GT_U32                       eVid; /*  The packet's VID or the VID assigned to the packet.. Set by Frame Parser, PCL.
                                            in eArch :
                                                Change: Instead of <VID0>. Extended to 16 bits.
                                            */
   GT_U32                       vlanEtherType; /* The packet's VID Ethertype. Set by Frame Parser */
   GT_BIT                       basicMode;  /* This bit indicates that the device operates as a 802.1D bridge or as a
                                               802.1Q Bridge. */
   struct{
        GT_U16      vid;
        GT_U8       cfi;
        GT_U8       vpt;
   }vlanTag802dot1dInfo;        /* TAG 0 : information saving the ingress vlan tag as it enters the device ,
                                   used when basicMode == 1 , or when need original tag info
                                   set in snetChtL2Parsing only for first parse (not modified from TTI re-parse)
                                   */

   GT_U32  vid0Or1AfterTti ;   /* <OrigVid0/1> -- VID 0/1 assigned to the packet after TTI stage
                                in ch3,xcat,LionB -- the value is only of OrigVid0
                                in xcat2 ,Lion2,Lion3 .. -- the value is OrigVid0 / OrigVid1
                                */

   GT_U32                       localDevSrcTrunkId;/* The local Device Source Trunk ID, Frame Parser,
                                in eArch :
                                Change: Extended to 12 bits
                                */
   GT_BIT                       srcDevIsOwn; /* the source device of the packet is own device, set by frame parser.*/
   GT_BIT                       egressFilterRegistered; /*The MAC table DA lookup result.Set by Frame Parser: -if packet is Marvell Tagged Extended FORWARD or FROM_CPU,<EgressFilter Registered> <= Mtag<EgressFilterRegistered> else, set to 1 (as if packet is registered) or L2i */
   GT_U32                       sstId; /*Source Span Tree ID for HP mash implementation, set by Frame Parser if packet is Marvell Tagged Extended FORWARD of FROM_CPU,<SST-ID> <= Mtag<SST-ID> else, set to 0 L2i - If packet is processed by the bridge:if MAC SA is found, <SST-ID> <= MAC_SA<SST-ID>else, <SST-ID> <= IngressPort<SST-ID>else, retain PCL setting*/
   GT_U32                       pclAssignedSstId;/*SIP5 - Used for Source ID assignment algorithm*/
   SKERNEL_PRECEDENCE_ORDER_ENT sstIdPrecedence; /* Source ID precedence */
   GT_U32                       pclSrcIdMask;/*SIP5 - PCL : Used for Source ID assignment algorithm*/
   SKERNEL_EXT_PACKET_CMD_ENT   packetCmd; /* packet forwarding command, set by frame parser, PCL, l2i, Policer, Eq, TxQ*/
   GT_BIT                       rxSniff; /* rx sniff indication,set by Frame Parser Pcl, If packet is Mtagged TO_CPU or FROM_CPU, <rx_sniff> <= 0 else, If packet is Mtagged TO_TARGET_SNIFFER set to Mtag<rx_sniff> else, according to Port or Action or L2i If the Bridge is not bypassed, according to: MAC or Vlan, else, InDesc<rx_sniff>*/
   SKERNEL_MIRROR_MODE_ENT      mirroringMode; /* mirroring mode to Analyzer for egress/ingress mirroring. For dxChXcat and above */
   GT_U32                       analyzerIndex; /* Analyzer index. Value 0 means there is no sniffer (relevant only when devObjPtr->supportMultiAnalyzerMirroring == 1)
                                                this index used for rx and for tx mirroring */
   SKERNEL_MIRROR_TYPE_BMP      mirrorType;/* mirror type : mirror / STC / both / none*/
   GT_BIT                       analyzerVlanTagAdd; /* Bit per analyzer port indicating if a VLAN Tag is to be added to TO_ANALYZER Packets */
   GT_BIT                       analyzerKeepVlanTag; /* Bit per analyzer port indicating if mirrored packets to analyzer may keep/add VLAN tag */
   GT_BIT                       isIp; /* packet is IPv4 or IPv6, set by Frame Parser */
   GT_BIT                       useVidx; /* 0 - packet forwarded to the Target port specified in this descriptor.
                                            1 - packet is forwarded to the multicast group specified in this descriptor,
                                             Set by Frame Parser from DSA Tag, PCL or L2i*/
   GT_U32                       eVidx; /* The multicast group to which the packet is to be transmitted.
                                         NOTE: setting VIDX[11:0] to 0xFFF indicates that the packet is forwarded to all members in the Vlan.
                                         Set By Frame Parser, PCL or L2i

                                         the eVidx replaces the 'old vidx' to support eArch.
                                         it supports 16 bits instead of 12.
                                         NOTE: the vidx field is still used in TXQ. see eArchExtInfo.vidx
                                         Note: When UseVIDX=1, EQ-->TxQ descriptor contains
                                            both eVIDX and 12-bit VIDX for per-port replication
                                         */
   GT_BIT                       targetIsTrunk; /* The Target of the packet is a Trunk, Set By PCL or L2i */
   GT_U32                       trgEPort; /* Target E Port, Set by Frame Parser from Marvel tag, PCL or L2i
                                         the trgEPort replaces the 'old trgPhyPort' to support eArch.
                                         in eArch : it supports 20 bits instead of 6.
                                         NOTE: the trgPhyPort field is still used in TXQ. see eArchExtInfo.trgPhyPort*/
   GT_U32                       trgDev;  /* Target Device, Set by Frame Parser from Marvel tag, PCL or L2i.
                                            NOTE: in eArch Extended to 12 bits */
   GT_U32                       trgTrunkId; /* The Target Trunk to which the packet is forwarded.
                                              Set By PCL, L2i.
                                            NOTE: in eArch Extended to 12 bits */
   GT_U32                       cpuCode; /*The packet CPU code, set by frame Parser from Marvell Tag, PCL or L2i*/

   /* Data from Marvell Tag */
   /* when <PacketCmd>! = FROM_CPU */
   GT_BIT                       routed; /* did the current device or previous device route the packet
                                        0  the packet was not routed.
                                        1  the packet is or was routed.

                                        SA modification indication
                                        */
   GT_BIT                       origIsTrunk; /*when set, packet's origin is from a trunk, else packet was received from a port
                                              , set by frame parser from Marvell Tag*/

   GT_U32                       origSrcEPortOrTrnk; /* Packet's Original Source port or Trunk, frame parser from Marvell Tag
                                        in EArch:
                                        when origIsTrunk == 0
                                            Change:  Instead of <OrigSrcPort>. Extended to 20 bits.
                                            For packets that arrive non-DSA, keep the existing semantics of
                                            OrigSrcPort where this field is always equal to LocalDevSrcPort (even
                                            when TTI assigns Src ePort).
                                            For packets that arrive with DSA, this field is assigned DSA<SrcPort> and
                                            may be overridden by TTI Src ePort assignment (e.g. central chassis)
                                        when origIsTrunk == 1
                                            Change: Extended to 12 bits
                                        */

   /*when <PacketCmd> = FROM_CPU */
   GT_BIT                       egressFilterEn; /* Enable Egress filtering, Set by frame parser from Marvell Tag */
   GT_BIT                       excludeIsTrunk; /* Indicates if the excluded target from the multicast group is a trunk or a port,
                                                   Set by frame parser from Marvell Tag */
   GT_U32                       excludedTrunk; /* The Excluded Trunk number,
                                                  Set by frame parser from Marvell Tag */
   GT_BIT                       excludedDevice;/* Together ExcludedPort, specifies the port that excluded from the Multicast group.
                                                Set by frame parser from Marvell Tag */
   GT_U32                       excludedPort;/* Together ExcludedDev, specifies the port that excluded from the Multicast group.,
                                                  Set by frame parser from Marvell Tag */
   GT_BIT                       mailBoxToNeighborCPU; /* CPU mail box from CPU to the neighbor CPU
                                                     Set by frame parser from Marvell Tag */
   GT_BIT                       mirrorToAllCpus; /* Mirror the packet to all CPUs Enable a CPU to send a Multicast/Broadcast packet to all CPUs in the system.
                                                    0 = The Multicast packet sent by the CPU is not mirrored to the CPU attached to the receiving device.
                                                    1 = The Multicast packet sent by the CPU is mirrored to the CPU attached to the
                                                        receiving device with a TO_CPU DSA Tag and MIRROR_TO_ALL_CPUS CPU Code.*/
   /* Data for Policer */
   GT_BIT                       policerEn; /* enable policer indication ,set by IPCL*/
   GT_BIT                       policerCounterEn; /* If set, the packet is bound to a counter specified in <policerPtr>. */
   GT_U32                       policerPtr; /* The policer pointer, valid when <PolicerEn> =1, set by PCL */
   GT_U32                       policerCntPtr; /* Obsolete use policerPtr instead !!!. The policer counter pointer. From Policer Entry <PolicerEn> = 1, or <policerPtr> from PCL action */
   GT_U32                       policerCycle; /* Current policer engine */
   GT_BIT                       policerEgressEn; /* enable policer indication, set by EPCL*/
   GT_BIT                       policerEgressCntEn; /* enable policer counting indication, set by EPCL*/
   SKERNEL_POLICER_TRIGGER_MODE_ENT policerTriggerMode;/* policer trigger mode : 'port mode' / 'flow mode' / 'ePort/eVlan mode' */
   GT_U32                       policerEArchPointer;/* EArch - policer pointer , relevant when policerTriggerMode = ePort/eVlan*/
   GT_BOOL                      policerEArchCounterEnabled;/* EArch - counter enabled , relevant when policerTriggerMode = ePort/eVlan*/
   GT_U32                       policerActuallAccessedIndex;/* actual policer index used to access the metering table */
   GT_U32                       countingActuallAccessedIndex;/* actual counting index used to access the metering table */
   GT_U32                       *policerCounterEntryMemoryPtr;/* pointer to the entry in the counter table (in current PLR)*/
   GT_U32                       policerCounterGlobalNumBytes;/*policer - global num of bytes */
   GT_U32                       *policerMeterConfigEntryMemoryPtr;/* sip5_15 : pointer to the entry in the meter config table (in current PLR)*/
   GT_U32                       *policerConformanceLevelSignEntryMemoryPtr;/* sip5_15 : pointer to the entry in the Policer Metering Conformance Level Sign table (in current PLR)*/
   /* Data needed by Bridge and Router */
   GT_U32                       pktHash;/* The Packet Hash used for Trunk load balancing and ECMP, Frame Parser, */
   GT_U32                       pktHashForIpcl;/* sip5 The Packet Hash used for IPCL keys only (it is subset of bits from pktHash) */
   SKERNEL_QOS_PROF_PRECED_ENT  qosProfilePrecedence; /*setting by PCL */
   GT_BIT                       ipHeaderError;/*is the Ipv4/Ipv6 hold error ?*/
   GT_BIT                       ipTtiHeaderError;/* extra TII limitation on the Ipv4/Ipv6 header, and SIP5 TTI exceptions checks */
   GT_BIT                       greHeaderError;/*the GRE header 2 bytes are not ZERO -- error */
   GT_BIT                       isIPv4; /* Is packet ipV4, set by frame parser */
   GT_BIT                       isNat; /* sip5_10 & sip5_15 only: Is packet ipV4 NAT, set by ipvx unit
                                          sip5_15 only: Is packet ipV6 NAT, set by ipvx unit */
   GT_U32                       sip[4]; /* Source IP address in the Network order-
                                           I.E sip[0] has 4 MSB of ipv4/ipv6
                                               and (sip[x] >> 24) hold MSB of word x
                                          for IPv4 only sip[0] is valid,*/
   GT_U32                       dip[4]; /* Destination IP address in the Network order,
                                           I.E dip[0] has 4 MSB of ipv4/ipv6
                                               and (dip[x] >> 24) hold MSB of word x
                                          for IPv4 only dip[0] is valid*/

   /* Data needed by Bridge */
   GT_BIT                       bypassBridge; /*When set, the Bridge decision, apart from learning, is bypassed.
                                                set by frame parser and PCL */
   GT_BIT                       ipm; /* IP Multicast , set by frame parser */
   GT_BIT                       arp; /* ARP , set by frame parser */
   GT_U32                       srcDev; /* Source device id, set by Frame Parser from Marvell Tag or from ownDevice register
                                         in eArch : it supports 12 bits instead of 5.*/
   GT_BIT                       igmpQuery; /* Set By frame Parser - 1) ethertype = 0x800. 2) IP protocol = 2 3) L4 byte 0 = 0x11*/
   GT_BIT                       igmpNonQuery; /* Set By frame Parser - 1) ethertype = 0x800. 2) IP protocol = 2 3) L4 byte 0 != 0x11*/
   GT_BIT                       portVlanSel; /* (NOT for sip5) Frame parser to PortControlReg<LocalDev SrcPort>[VlanSel] */
   GT_BIT                       solicitationMcastMsg; /* setting by frame parser. Packet EtherType = IPV6 and DIP[127:0] * Solicited-Node Multicast address Mask =
                                                     Solicited-Node Multicast address prefix* Solicited-Node Multicast address Mask. */
   GT_BIT                       ipv4Icmp; /* IPv4 ICMP frame, if the packet ethertype = 0x800 (IPv4) and the IPv4 protocol ICMPv4 (1).*/
   GT_BIT                       ipv6Icmp; /* IPv6 ICMP frame, if the packet ethertype = 0x86DD (IPv6) and the IPv6 next header is ICMPv6 (58).*/
   GT_U32                       ipv6IcmpType; /* IPv6 ICMP frame, taken from offset 40B from IPv6 Header.*/
   GT_BIT                       isIpV6EhExists;   /* indicates whether at least one of extended headers exists */
   GT_BIT                       isIpV6EhHopByHop; /* indicates whether ipv6 hop by hop extension exists */
   GT_BIT                       isIpv6Mld;
   GT_BIT                       ipXMcLinkLocalProt; /* for IPv4: DIP[31:0] = 224:00:00:xx, for IPv6: DIP[127:0] = FF02:0:0:0:0:0:0:xx */
   GT_BIT                       ipv4Ripv1; /* 1.MAC DA = ff-ff-ff-ff-ff-ff 2. DIP = 255:255:255:255 3. Prot = 17 (UDP). 4. UDP Dest Port=520 (RIPv1). 5. IPv4.*/

   SKERNEL_L2_ENCAPSULATION_TYPE_ENT    l2Encaps;
   GT_U32                       etherTypeOrSsapDsap; /* real ether type for Ethernet II or SSAP DSAP for LLC frames */
   GT_U32                       dscp; /* DiffService code point */
   GT_U32                       ipProt; /* IP Protocol */
   GT_BIT                       ipv4DontFragmentBit;/* the ipv4 header bit of 'dont fragment' */
   GT_BIT                       ipv4HeaderOptionsExists;/* does the ipv4 header includes 'options' */
   GT_U32                       ipv4FragmentOffset; /* ipv4 fragment offset field value*/


   SNET_CHEETAH_L2I_VLAN_INFO   ingressVlanInfo;/* ingress vlan info */
   SNET_CHEETAH_EGR_VLAN_INFO   egressVlanInfo;/* egress vlan info */
   /* EQ Set fields below */
   SKERNEL_MTAG_CMD_ENT         outGoingMtagCmd;  /*The packet Egress forwarding command.setting by: Eq as follows:
                                                    3 - FORWARD descriptor with <OutGoing MtagCmd> = FORWARD is sent to Txq when one of
                                                       the following is true:
                                                        1. <PacketCmd> = Forward and the Target port is not the CPU
                                                        2. <PacketCmd> = MirrorToCPU and the Target port is not the CPU
                                                    2 - TO_TARGET_SNIFFER a descriptor with <OutGoing MtagCmd> = TO_TARGET_SNIFFER is sent to Txq
                                                        when one of the following is true:
                                                        1. <PacketCmd> = ToTargetSniffer and <rx_sniff> = 1 and RxMonitorPort!= 0x3F
                                                        - In this case the descriptor is not duplicated.
                                                        2. <PacketCmd> = ToTargetSniffer and <rx_sniff> = 0 and TxMonitorPort!= 0x3F
                                                        - In this case the descriptor is not duplicated
                                                        3. <PacketCmd>!= ToTargetSniffer and packet received from the Ingress core
                                                        and <rx_sniff> = 1 and RxMonitorPort!= 0x3F
                                                        - In this case the descriptor is duplicated.
                                                        4. Descriptor received from the Txq that is not ExRmon, <TxSampled> = 0, and
                                                        is to be sent to the Target Tx monitor and TxMonitorPort!= 0x3F
                                                    1 - FROM_CPU a descriptor with <OutGoing MtagCmd> = FROM_CPU is sent to
                                                        Txq when <PacketCmd> = FromCPU and one of the following is true:
                                                        1. <MailBoxTo NeighborCPU> = 1 and <LocalDev SrcPort>== 0x3F
                                                        2. !(<TrgPort> ==0x3F and <TrgDev> == OwnDev)
                                                    0 - TO_CPU see: <CPUCode>*/
   GT_U32                       tc;  /* the packet's TC */
   GT_U32                       dp;  /* the packet's DP */
   GT_U32                       meterDp; /* the packet's DP assigned by metering engine */
   GT_BIT                       truncated; /* If 1 - send only 128 first bytes */
   GT_U32                       cpuTrgDev; /* setting by: Eq - According to CPU code table indexed by <CPUCode>
                                            Used by:
                                            Txq - to queue the packet as follow:
                                            if <CPUTrgDev> == OwnDev, the packet is queued to the local CPU port (0x63)
                                            else the packet is queued according to
                                            TargetDev2LocalPort/TrunkTable[<CPUTrgDev>]*/
   GT_BIT                     srcTrg; /* Indicates source or Target Data is forwarded to the CPU
                                         0 - source (ingress pipe) to cpu
                                         1 - target (egress pipe) to cpu
                                         setting by: Eq -
                                            if packet is forwarded to the CPU due to Egress sampling or monitoring (a descrip-tor
                                            from the txq) and EgressSamplingIngressDataEn = 0, set to 1 else set to 0
                                         Used by:  RdDMA-HA- to prepare the TO_CPU marvell tag as follows:
                                            If the Incoming Mtag is extended TO_CPU Mtag<SrcTrg> is taken from the pkt, else
                                            Mtag<SrcTrg> is taken from <SrcTrg>*/
   GT_BIT                    srcTaggedTrgTagged; /* setting by: Eq - If <SrcTrg> = 1, set to Txq2EqDesc<TrgTagged>, else set to <SrcTagged>
                                        Used by:
                                            RdDMA-HA- to prepare the TO_CPU marvell tag as follows:
                                            If the Incoming Mtag is TO_CPU Mtag<SrcTagged/TrgTagged> is taken from the
                                            pkt, else Mtag<SrcTagged/TrgTagged> is taken from <SrcTagged/ TrgTagged>*/
   GT_U32                   srcTrgPhysicalPort ; /*  setting by: Eq -
                                            If <SrcTrg> = 1, set to Txq2EqDesc<EgressTrgPort>, else set to:
                                            If InDesc<MailBoxTo NeighborCPU> == 1, set to <LocalDev SrcPort>
                                            else set to <OrigSrcPort>
                                        Used by:
                                            RdDMA-HA- to prepare the TO_CPU marvell tag as follows:
                                            If the Incoming Mtag is TO_CPU Mtag<SrcPort/TrgPort> is taken from the pkt
                                            else Mtag<SrcPort/TrgPort> is taken from <SrcPort/ TrgPort>
                                         in eArch : it supports 8 bits instead of 6.*/
   GT_U32                   srcTrgDev ; /*  setting by: Eq - to OwnDev
                                        Used by:
                                            RdDMA-HA- to prepare the TO_CPU marvell tag as follows:
                                            If the Incoming Mtag is TO_CPU Mtag<SrcDev/TrgDev> is taken from the pkt
                                            else Mtag<SrcDev/TrgDev> is taken from <SrcDev/ TrgDev> and set to OwnDev
                                         in eArch : it supports 12 bits instead of 5.*/
   GT_U32                   sniffTrgDev; /* setting by: Eq -
                                            if <rx_sniff> == 1, <SniffTrgDev> <= RxMonitorDev
                                            else <SniffTrgDev> <= TxMonitorDev
                                        Used by:
                                            Txq - to forward the packet to its destination
                                            If <SniffTrgDev> == OwnDev, forward the packet to <Sniff TrgPort>,
                                            else the packet is queued according to
                                            TargetDev2LocalPort/TrunkTable[<SniffTrgDev>]

                                            eArch : 12 bits
                                            */
  GT_U32                   sniffTrgPort; /* setting by: Eq -
                                            if <rx_sniff> == 1, <Sniff TrgPort> <= RxMonitorPort
                                            else <Sniff TrgPort> <= TxMonitorPort
                                            Used by:
                                            Txq - to forward the packet to its destination

                                            eArch : 8 bits
                                            */

  /* Tx Mirroring and Sampling related bits. Set by TXQ unit. Used by EQ. */
  GT_BIT                  txSampled; /* The reason for forwarding the descriptor to Eq
                                        0 - Tx sniffed
                                        1 - Tx sampled*/
  GT_BIT                  txMirrorDone;  /* Packet has been TX mirrored */
  GT_U32                  egressTrgPort; /* The Egress port through which the packet is transmitted
                                            NOTE: this field is valid ONLY for
                                            Tx mirroring and used only by the EQ -->
                                            see note about field : <srcTrgPhysicalPort>

                                            set by TXQ for 'tx Sniff' when sent to EQ
                                            used by EQ for 'tx Sniff' when got from TXQ
                                            */

  /* Cheetah+ specific UC routing set by l2 and PCL */
  GT_BIT                  bridgeUcRoutEn;    /* Bridge IPv4/V6 Unicast Routing is enabled/disabled  */

  /*  Cheetah2 specific MC routing set by l2 and Routing */
  GT_BIT                  bridgeMcRoutEn;    /* Bridge IPv4/V6 Multicast Routing is enabled/disabled  */

  /* Application-Specific CPU Codes */
  GT_BIT                  appSpecCpuCode;   /* Enable application-specific CPU Code assignment mechanism */

  GT_U32                  pclUcNextHopIndex; /* PCL action index to next hop */
  GT_BIT                  doRouterHa;        /* indicates that packet routed indeed by own device or remote device.
                                                0  the local device not routed the packet
                                                1  the local device routed the packet
                                                DA modification indication --> uses 'ARP pointer'
                                                also allow IP header changes : TTL,DSCP
                                                */
  GT_U32                  flowLabel;         /* Used for specifying special router handling from source to destination(s) for a sequence of packets */
  GT_BOOL                 decTtl;            /* Decrement TTL */
  GT_U8                   ttl;               /* TTL value from router entry is used
                                               to get result TTL in the egress header alteration */
  GT_U8                   ttl1;              /* TTL value from MPLS label1 - is relevant only for MPLS packets */
  GT_U8                   ttl2;              /* TTL value from MPLS label2 - is relevant only for MPLS packets */
  GT_U8                   ttl3;              /* TTL value from MPLS label3 - is relevant only for MPLS packets */
  GT_U8                   ttl4;              /* SIP5: TTL value from MPLS label4 - is relevant only for MPLS packets */
  GT_U8                   cwFirstNibble;     /* First 4 bits of data after the bottom of the label stack -
                                                to support Ethernet over MPLS .

                                                value 0 means ethernet over MPLS
                                                value 4 means IPv4 over MPLS
                                                value 6 means IPv6 over MPLS
                                                */
  GT_BIT                  mplsUseSpecificTtlForTtlException;/*indication that the for TTL exception checks TTL in the descriptor->ttl is NOT to be used when
                                                mplsUseSpecificTtlForTtlException = 1 , because the descriptor->ttl may hold bogus value that not from actual label
                                                --> implemented to match VERIFIER code */
  GT_U32                  mplsLabelTtlForException;/* the TTL to be used for TTL exception check when mplsUseSpecificTtlForTtlException = 1 */

  GT_U32                  arpPtr;            /* Destination address index in ARP table.
                                                in eArch : 22 bits

                                                valid only when : descriptor<tunnelStart> = 0 */
  GT_BIT                  modifyMacDa;       /* When this flag is enabled,
                                                the destination MAC address of the packet is altered according to the
                                                ARP entry specified in the <arpPtr>.

                                                NOTE: this field can be used only in the 'ingress pipe' because in
                                                       the Egress it is combined into <doRouterHa> (function snetChtTxQPhase1)
                                                */
  GT_BIT                  modifyMacSa;       /* When this flag is enabled,
                                                the source MAC address of the packet is altered to the address of the current device
                                                NOTE: this field can be used only in the 'ingress pipe' because in
                                                       the Egress it is combined into <routed> (function snetChtTxQPhase1)
                                                */
  /* cheetah2*/
  GT_BOOL                 macDaFound;    /* (1-Mac DA is found , 0 - else */
  GT_U32                  udpBcCpuCodeInx;/* index to the cpu code in UDP broadcast destination  port table - cht2 only*/
  GT_U32                  saAccessLevel; /* SA security access Level */
  GT_U32                  daAccessLevel; /* DA security access Level */
  GT_BIT                  validMll;      /* do we have valid MLL */
  GT_U32                  mll;           /* mll address index in MLL table*/
  SKERNEL_MLL_SELECT_ENT  mllSelector;   /* mll selector for ipv6 : 1 - internal mll (mll) 0 - external mll (mllexternal)
                                            use MLL_SELECT_ENT
                                            NOTE: invert to value in register */
  GT_U32                  mllexternal;   /* mll address index in MLL table*/
  PCL_TTI_ACTION_REDIRECT_CMD_ENT    pclRedirectCmd;   /* PCL redirect command  */
  PCL_TTI_ACTION_REDIRECT_CMD_ENT    ttiRedirectCmd;   /* TTI redirect command  */
  GT_U32                  ipV4ucvlan;    /* bit to make route on Vlan  IPv4UC */
  GT_U32                  ipV4mcvlan;    /* bit to make route on Vlan  IPv4MC */
  GT_U32                  ipV6ucvlan;    /* bit to make route on Vlan  IPv6UC */
  GT_U32                  ipV6mcvlan;    /* bit to make route on Vlan  IPv6MC */
  GT_BIT                  fdbBasedUcRouting;/* indicate that the Router engine can perform FDB DIP Lookup*/
  /* Data needed by tunneling */
  GT_BIT                  tunnelStart;   /* indication packet need to start tunneling for egress */
  SKERNEL_FRAME_TUNNEL_START_TYPE_ENT tunnelStartType;   /* the tunnel start type */
  GT_BIT                  tunnelStartPassengerType; /* Type of passenger packet being encapsulated.
                                                       NOTE: This field is valid only when
                                                       <Redirect Command>= 1 (Policy-switching) and
                                                       <TunnelStart> = 1
                                                       NOTE: When packet is not TT and
                                                       <MPLS Command> != NOP, the <Tunnel Type> bit
                                                       must be set to OTHER.
                                                       0x0 = Ethernet;
                                                       0x1 = Other; */
  GT_U32                  numOfLabels;   /* Number of MPLS labels in the ingress Packet (0 based --> 0 - means one label , 1 - means 2 labels..) */
  GT_BIT                  bypassRouter;  /* When set, the Router decision, is bypassed.*/
  GT_BIT                  bypassIngressPipe; /* When set, the packet bypasses the bridge and router
                                                engines. In this case the bridge is completely
                                                bypassed, including source address learning */
  GT_BIT                  mpls;          /* indication packet ie tunnelled    */
  GT_U32                  tunnelPtr;     /* tunnel start index in ARP table  , valid when tunnelStart = 1
                                            in eArch: 20 bits
                                         */
  GT_U32                  label1;        /* first mpls label                  */
  GT_U32                  exp1;          /* first mpls label                  */
  GT_U32                  label2;        /* first mpls label                  */
  GT_U32                  exp2;          /* second  mpls label                */
  GT_U32                  label3;        /* third mpls label                  */
  GT_U32                  exp3;          /* third  mpls exp                   */
  GT_U32                  label4;        /* sip5 : fourth mpls label          */
  GT_U32                  exp4;          /* sip5 : fourth  mpls exp           */
  GT_BIT                  setSBit;       /* MPLS Label S bit:
                                            0x0 - If clear, all TS labels have the S-bit clear
                                            0x1 - The inner label of the TS entry has its S-bit set.
                                                  The outer labels have S-bit clear.*/
  GT_BIT                  mplsReservedLabelExist;   /* Indicates that a reserved label (0-15) exists in one of the first 4
                                                       labels in the MPLS stack*/
  GT_U32                  mplsReservedLabelValue;   /* If a Reserved Label was found in the top 4 labels of the label stack,
                                                       this field contains the value of the first reserved label found.*/
  GT_U32                  mplsPayloadData;          /* This field contains the 5 bits that immediately follow the bottom of label stack.
                                                       If there is a Control Word, this field contains the 5 ms bits of the Control Word */
  SKERNEL_XCAT_TTI_MPLS_COMMAND_ENT  mplsCommand;   /* xcat and above : mpls command defined in tti action */
  GT_U8 *                 afterMplsLabelsPtr;   /* pointer to payload data after MPLS header parsing */
  GT_BIT                  tsEgressMplsControlWordExist; /* PWE3 Control word added by MPLS tunnel start */
  GT_U32                  tsEgressMplsNumOfLabels; /* Number of MPLS lables added by MPLS tunnel start (0 based --> 0 - means one label , 1 - means 2 labels..)*/
  GT_U32                  origTunnelTtl; /* save tunel ttl for use in tunnel start */
  GT_BIT                  mim;           /* indication packet is IEEE 802.1ah */
  GT_U32                  iUp;           /* inner user priority field (PCP) in the iTag */
  GT_BIT                  iDp;           /* inner drop precedence in the iTag <DEI> */
  GT_U32                  iRes1;         /* 2 reserved bits to be set in the iTag */
  GT_U32                  iRes2;         /* 2 reserved bits to be set in the iTag */
  GT_U32                  iSid;          /* Backbone Service Instance Identifier in the iTag */
  GT_BIT                       policyOnPortEn; /* Policy processing enabled for a port */
  SKERNEL_PCL_LOOKUP_MODE_ENT pclLookUpMode[3]; /* Controls the index used for IPCL lookups :
                                                index 0 - lookup 00
                                                index 1 - lookup 01
                                                index 2 - lookup 10
                                                */
  GT_U32                  ipclProfileIndex; /* Pointer to IPCL configuration entry to be used when
                                               fetching IPCL parameters. This may be used to overrides
                                               the Port/VLAN based configuration entry selection
                                               based on the <PCL[n] Override Config Index> */

  /* cheetah3 */
  GT_BIT                  mac2me;        /* MAC2ME table Lookup */
  GT_U32                  tunnelTerminated;  /* tunnel termination indication */
  GT_U32                  pceRoutLttIdx; /* index for the Router Lookup Translation table,
                                            instead of the Index produced from
                                            the DIP LPM lookup in the routed TCAM. */
  GT_U32                  vrfId;         /* VRF-ID */
  GT_U32                  VntL2Echo;     /* MAC SA and MAC DA switched in transmission */
  GT_U32                  ActionStop;     /* if set packet will have no search in IPCL  */
  GT_U8                  *l3StartOfPassenger; /* pointer to the start of passenger in T.T */
  GT_U32                  passengerLength;/* length of the passenger without the tunnel */
  GT_U32                  protOverMpls;  /* protocol above MPLS               */
  GT_U32                  cfidei;        /* cfi bit in VLAN                   */
  GT_U32                  up1;           /* user priority in VLAN             */
  GT_U32                  cfidei1;       /* cfi bit in VLAN                   */
  GT_U32                  vid1;          /* tag1 VID                          */
  GT_U32                  originalVid1;  /* ingress tag1 VID value            */
  GT_U32                  vlanEtherType1; /* The packet's VID Ethertype. Set by Frame Parser */
  GT_BIT                  trustTag1Qos;   /* when L2 trust used - define if using up,cfi or up1,cfi1 */

  GT_BIT                  oam;           /* OAM indication                    */
  GT_BIT                  cfm;           /* CFM packet                        */
  GT_BIT                  pktIsLooped;     /* packet is looped fast failure   */
  GT_BIT                  dropOnSource;    /* Drop packet on its source device indicator, for Fast Stack Fail over Recovery Support.
                                              This bit is set to 1 when:
                                              * <Packet Is Looped> = 1 and <Src Dev> = OwnDev, and in the incoming DSA tag <Drop On Source>=0
                                              * Packet received with <Src Dev> = OwnDev and <Drop On Source> = 1 is dropped   */

  GT_U16                  l4SrcPort; /* L4 source port - TCP / UDP */
  GT_U16                  l4DstPort; /* L4 destination port - TCP / UDP */

  GT_BIT                  udpCompatible;/* UDP/Udp-Lite */
  GT_U32                  cpuTrgPort; /* setting by: Eq - According to CPU code table indexed by <CPUCode>
                                            Used by:
                                            Txq - to queue the packet as follow:
                                            used the designated port number.
                                            this is valid only when <CPUTrgDev> == OwnDev
                                            */

  SKERNEL_FRAME_CHEETAH_DESCR_CAPWAP_STC capwap;/*capwap info - Control and Provisioning of Wireless Access Points */

  GT_U32                  ieeeReservedMcastCmdProfile;/*There are 2 global IEEE Reserved Multicast Command profile.*/

  GT_U32                  mllPtrForWlanBridging;/* if != 0 then this is the address of mll to send replication of traffic to it
                                               it may be mll of the vlan or mll of the vidx */
  GT_BIT                  passengerTag;/* the Passenger tagged/untagged - how to build in the tunnelStart */
  GT_BIT                  validSrcIdBmp;/*is the srcIdBmp valid or not valid */
  GT_U32                  srcIdBmp;/* 32-bit SrcID filter enable bitmap.
                                      If bit n is 0, the packet that is destined to this logical port and that
                                            been assigned SrcID n is filtered.
                                      If bit n is 1, the packet that is destined to this logical port and that
                                            been assigned SrcID n is forwarded.*/
  GT_BIT                  ignoreQosIndexFromDsaTag;/* ignore QosIndex from DSA tag */
  GT_BIT                  egressOsmRedirect;/* the egress frame should go to the CPU
                                        and not to original egress port due to OSM redirect */
  GT_U8                   *tunnelStartMacInfoPtr;
  GT_U32                  tunnelStartMacInfoLen;

  GT_U8                   *tunnelStartRestOfHeaderInfoPtr; /*start with the EtherType !!*/
  GT_U32                  tunnelStartRestOfHeaderInfoLen;

  GT_U32 ethernetOverXPassengerTagMode;/* The VLAN Tag mode of the passenger packet for an Ethernet-Over-xxx tunnel
                                   Start packet.
                                   0 = the_Tag: VLAN Tag mode (Tagged or Untagged) is set according to the Tag
                                                bit in the VLAN Entry
                                   1 = <EthernetOverXPassengerTagged>.: VLAN Tag mode (Tagged or Untagged) is set
                                                according to <EthernetOverXPassengerTagged>.*/
  GT_U32 ethernetOverXPassengerTagged;/* The tag format of the passenger packets of Ethernet-Over-xxx Tunnel Start
                                        packets, valid when <EthernetOverXPassangerTagMode> = 1.
                                        0 = Untagged: Passenger packets in an Ethernet-Over-xxx Tunnel Start packets
                                                        are untagged.
                                        1 = Tagged: Passenger packets in an Ethernet-Over-xxx Tunnel Start packets are tagged.
                                    */

  /* Data needed by tunneling */
  GT_U32                  ttRouterLTT;/* pointer to Router LTT entry */
  GT_U8                   udb[3];    /*  user defined bytes 0,1,2 : used by CFM packet MD level , Opcode  , Flags  */


  GT_BOOL                 ipServiceFlag; /* IP flag for special service */
  GT_BOOL                 brdgUcIpHeaderCheck; /* IP Header Check for Bridged Unicast IPv4/6 Packets */

  GT_BOOL                 useIngressPipeVid;/* do we need to use ingressPipeVid.
                                            On trapping/mirroring packets to CPU or egress sniffer
                                            indicates which vid (modified in ingress or in egress pipe) is used.
                                            useIngressPipeVid - GT_TRUE is default at the beginning of ingress pipe
                                            useIngressPipeVid - GT_FALSE reset by the egress pipe when frames are
                                            trapped/mirrored to CPU or mirrored to egress analyzer */
  GT_U16                  ingressPipeVid;/* vid that assigned to packet in the
                                            ingress core , before the ROUTER
                                            this vid is used for frames that are
                                            mirrored to CPU , and mirrored to
                                            ingress analyzer */

  GT_U32                  egressByteCount;/* egress frame size */

  /* TR101 support (xCat and above) */
  SKERNEL_FRAME_TR101_VLAN_TAG_CLASS_ENT ingressVlanTag0Type; /* tag 0 type from the ingress pipe */
  SKERNEL_FRAME_TR101_VLAN_TAG_CLASS_ENT ingressVlanTag1Type; /* tag 1 type from the ingress pipe */
  GT_U8                     * tag0Ptr;/*pointer to tag 0 in the ingress frame - set for packet with tag 0
                                        can be set again for tunnel termination

                                        = NULL when no tag 0 -->see ingressVlanTag0Type
                                        */
  GT_U8                     * tag1Ptr;/*pointer to tag 1 in the ingress frame - set for packet with tag 1
                                        can be set again for tunnel termination

                                        = NULL when no tag 1 -->see ingressVlanTag1Type
                                        */
  GT_U32                      tpidIndex[SNET_CHT_TAG_MAX_INDEX_CNS];/* tpid index for TPID0,TPID1*/
  GT_U32                      tpidIndexTunnelstart;/* tpid index for Tunnel start*/
  GT_BIT                      tagSrcTagged[SNET_CHT_TAG_MAX_INDEX_CNS];/*Indicates whether the packet was originally received with Tag0 , Tag1
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse */
  GT_BIT                      tag1LocalDevSrcTagged;/*Indicates that the packet inner/outer ethertype matches one of the local dev ingress port Tag1 TPIDs
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse */
  GT_BIT                      tag0IsOuterTag;/*srcTag0IsOuterTag - When the packet is tagged with both Tag0 and Tag1 (<Tag0 Src Tagged> = 1 and <Tag1 Src Tagged> = 1), indicates which of the two tags is the outer tag
                                                 set in snetChtL2Parsing --> modified from first parse and tti re-parse */

  GT_BIT                  tr101ModeEn;  /* TR-101 Configuration */

  GT_U32                  localPortGroupPortAsGlobalDevicePort;/*
                            on ingress : global port
                            on egress  : local port

                            for non-port group device , it is the same value as  localDevSrcPort
                            the local port number
                            from which the packet entered the port group device ,
                            but converted to 'global' port number , for the
                            'port group shared' device .
                            for example:
                            in Lion port group 3 , local port 5 is 'Global port' 53 ((3*16)+5)

                            --> used instead of : localDevSrcPort for:
                            bridging : source filtering of port from trunk

                            this set during 'Ingress port processing'

                            on egress pipe:
                            for non-port group device , it is the same value as  localDevSrcPort.
                            multicast source filtering , OAM loop back filter

                            */
  GT_U32                  localPortTrunkAsGlobalPortTrunk;/*
                            the local port number (or trunk number)
                            --> used instead of : origSrcPortOrTrnk for:
                            bridging : moved addresses , learning , source filtering

                            on egress pipe:
                               multicast source filtering , OAM loop back filter

                            */

  GT_BIT                  egressOnIngressPortGroup;/*
                            for non-port group device , it is always 1

                            1 - the packet egress from the same port group as it ingress
                            0 - the packet egress from a port group different from it ingress
                            */

  GT_U32                  ipFixTimeStampValue; /* Policer IPFix Time stamp value */

  SNET_CHEETAH_AFTER_HA_COMMON_ACTION_STC   epclAction;/* the action that EPCL set */
  SNET_CHEETAH_AFTER_HA_COMMON_ACTION_STC   eplrAction;/* the action that EPLR set */
  SNET_CHEETAH_AFTER_HA_COMMON_ACTION_STC   haAction;  /* the action that HA set */
  GT_BIT    portIsRingCorePort;/* is ingress port considered 'ring port' -- multi-lookups */
  GT_U32    srcCoreId; /* indication from which PortGroupId  the packet ingress the device,
                                it may be the local port group Id (when ingress port is not ring port)
                                or a remote port group Id (when ingress port is ring port and value parsed from the DSA)
                                this field will be use in :
                                - TTI build of key
                                - EQ to know what is the next ring port/trunk , unknown command
                                - HA when build DSA tag that egress from ring port */
  GT_U32    ttiHashMaskIndex; /* TTI action hash mask index   */
  GT_BIT    forceNewDsaToCpu; /* If enabled, a VLAN tagged packet is sent to the CPU with both the original VID and the modified VID */
  GT_BIT    bypassRouterAndPolicer; /* When enabled, the descriptor bypasses the Router and Policer.
                                    This reduces the packet latency in cut-through mode. */
  GT_U32    packetTimestamp; /* 32 bit timestamp specifying the nanosecond field of the TOD counter.
                                In ingress timestamping the timestamp marks the arrival of the beginning of the PTP packet,
                                while in egress timestamping it marks the beginning of packet transmission */

  SKERNEL_TIMESTAMP_TAG_TYPE_ENT timestampTagged[2];   /* Type of Timestamp Tags - Ingress & Egress */

  SNET_TIMESTAMP_TAG_INFO_STC    timestampTagInfo[2];  /* Timestamp Tag Content - Ingress & Egress*/

  SNET_TIMESTAMP_ACTION_INFO_STC timestampActionInfo;  /* EGRESS - timestamp action pararmeters */

  SNET_TOD_TIMER_STC timestamp[2];                /* Ingress & Egress TOD based on selected TAI */
  GT_BIT  ptpUField;                              /* lbs of update counter based on selected TAI */
  GT_BIT  ptpTaiSelect;                           /* Ingress selected TAI - 0 or 1 */
  GT_BIT  ptpEgressTaiSel;                        /* Egress selected TAI - 0 or 1 */
  GT_BIT  ptpIsTimestampLocal;                    /* Indicates whether the ingress timestamp arrived from another device through a timestamp tag or captured in the current device */

  GT_BIT    isPtp;           /* packet identified as PTP by TTI action */
  GT_BIT    isPtpException;
  SKERNEL_PTP_TRIGGER_TYPE_ENT    ptpTriggerType;  /* if (ipPtp) specifies the PTP Trigger Type:
                                over Ethernet, IPv4 UDP or IPv6 UDP */
  GT_U32    ptpOffset;       /* if (isPtp) defines byte offset to the start of the PTP header,
                                relative to the start of the L3 header */
  GT_U32    ptpDomain;       /* PTP domain entry number */

  GT_BIT    ptpActionIsLocal; /* Indication to the PTP action configuration place*/

  SNET_PTP_UDP_CHECKSUM_MODE_ENT ptpUdpChecksumMode;

  SNET_LION_PTP_GTS_INFO_STC ptpGtsInfo;  /* PTP related info */
  SNET_XCAT_IPFIX_ERRATA_STC ipfixErrataData;

  SKERNEL_E_ARCH_EXT_INFO_STC eArchExtInfo;/* eArch extra info */

  GT_U32          flowId;
  GT_U32          tunnelTerminationOffset;/*relevant when descrPtr->eArchExtInfo.packetIsTT */

    SNET_PER_PROTOCOL_INFO_STC perProtocolInfo;

    GT_BIT                    txqToEq;/* is this descriptor send from the TXQ-->EQ ?
                                        set by the TXQ. and used by the TXQ. */
    struct SKERNEL_FRAME_CHEETAH_DESCR_STCT* origDescrPtr;/* pointer to original descriptor that caused the TXQ-->EQ-->TXQ
                                        relevant when txqToEq = 1.

                                        pointer type is SKERNEL_FRAME_CHEETAH_DESCR_STC */
    SKERNEL_HA_TO_EPCL_INFO_STC haToEpclInfo;/* info needed by EPCL after the HA build the packet */
    SKERNEL_DEVICE_OBJECT       *ingressDevObjPtr;/* pointer to the ingress device object .
                                            for single core device --> the local device
                                            for multi-core device --> the ingress core device.
                                            */
    GT_U32                      extraSrcPortBits;/* extra src port bits --> in HW descriptor called
                                                    <SrcOwnDev0> , should be used with 'srcPort' as :
                                                    <srcPort> | < extraSrcPortBits > */
    GT_U32                      txqId; /* the TXQ unit (for flooding and single destination) - relevant in multi-TXQ units */
    GT_BIT                      forceToCpuTrgPortOnHemisphare0;/* do we need to force the traffic to CPU to go to cpu only from hemisphere 0 */
    GT_BIT                      floodToNextTxq;/*indication to flood to the next TXQ unit - relevant in multi-TXQ units*/
    GT_BIT                      dsaCoreIdBit2;/* bit from the DSA tag that represent bit 2 of srcCoreId - relevant to <dualDeviceIdEnable> and multi-core FDB .
                                                Parsed from the DSA , and used in the ingress in snetLionIngressSourcePortGroupIdGet */

    GT_BIT                      isTrillEtherType;/* is the packet considered with TRILL ethertype */
    GT_BIT                      trillEngineTriggered;/* is the packet triggered by the TRILL engine?
                                If the TRILL engine is triggered, the following features are
                                    based on the parsing of the inner Ethernet packet.
                                This is done regardless of whether the packet is tunnel-terminated or not.
                                - QoS Trust Mode
                                    Trust L2 and/or L3 QoS is based on the inner Ethernet packet
                                - IPCL key fields
                                    All IPCL key field are based on the inner Ethernet packet.
                                    This includes all fixed fields and user-defined bytes (UDBs).
                                    The UDB layer 2 anchor begins with the inner packet MAC DA.
                                - Packet hash key
                                    The packet hash key is based on the inner Ethernet packet.
                                    This includes all fixed fields and user-defined bytes (UDBs).
                                    The UDB layer 2 anchor begins with the inner packet MAC DA
                                */

    GT_BIT      tunnelStartTrillTransit;/* is the tunnel start is for 'TRILL transit' frame.
                            0 - TRILL tunnel start for 'native' packets
                            1 - TRILL transit that need HA to update the hopCount

                            set in the TS section of HA and used later in the HA frame build.

                            NOTE: relevant only when descrPtr->tunnelStartType ==
                                    SKERNEL_FRAME_TUNNEL_START_TYPE_TRILL_E
                            */

    SNET_TRILL_INFO_STC trillInfo;

    SNET_INGRESS_TUNNEL_INFO_STC ingressTunnelInfo;

    GT_BIT  ttiLookupMatch;/*did the TTI matched the packet in lookup*/

    GT_BIT  centralizedChassisModeEn;/* the packet was received from :
                            1. cascade port
                            2. port in mode - centralized Chassis Mode Enabled
                            3. DSA tag - 2 words
                            relevant only in eArch*/

    GT_U32  trustQosMappingTableIndex;/*    There are 12 global Trust QoS Mapping Table used for Trust L2, L3, EXP, DSCP-to-DSCP mapping.
                                            This field selects which of the 8 global sets of Trust QoS Mapping tables is used
                            relevant only in eArch*/
    GT_BIT  up2QosProfileMappingMode;/*Selects the index used to select a specific UP2QoS-Profile mapping table
                        0: The index is <Trust QoS Mapping Table Index>
                        1: The index is UP0 or UP1 as the UP configured for indexing a specific UP2QoS-Profile table. In this mode only tables 0-7 can be accessed
                            relevant only in eArch*/

    GT_U32  numOfBytesToPop;/* Indicates the number of bytes to pop off the packet
                                Used for EVB/BPE(802.1br)/6-bytes v-Tag.
                                0 = Do not pop a tag
                                1 = Pop tag of 4 bytes --> AKA EVB
                                2 = Pop tag of 8 bytes --> AKA BPE (802.1BR)
                                3 = Pop tag of 6 bytes (new in sip5_15) --> 6-bytes v-Tag
                              set by the TTI unit and used by the HA.
                            relevant only in eArch*/
    GT_BIT      popTagsWithoutReparse;/* indicate that the 'numOfTagsToPop' was set in descriptor without actual 'L2 re-parse'
                            the HA unit uses this info when build egress packet */

    GT_BOOL     didPacketParseFromEngine[SNET_CHT_FRAME_PARSE_MODE_LAST_E];/*
                            array of sources of packet parsing.

                            index to the array is source of packet parsing
                            value is GT_BOOL :
                                GT_TRUE - this source of packet parsing - did parsing.
                                GT_FALSE - this source of packet parsing - did NOT do parsing.

                            needed to bypass duplications of the same source of parsing.
                            */
    GT_BOOL      calcCrc;    /* GT_TRUE - calculate CRC value for Tx ethernet frames. No recalculation for DMA packets to CPU!!!  */
    SKERNEL_OAM_INFO_STC oamInfo; /* OAM related info */
    GT_BOOL      isFromSdma; /* GT_TRUE - frame received through DMA mechanism */

    SKERNEL_VPLS_INFO_STC   vplsInfo;
    GT_BIT                  overrideVid0WithOrigVid;

    struct{
        GT_U32                  virtualSrcPort;/*<VirtualSrcPort> together with <VirtualSrcDev> are used for Bridging and
                                                learning instead of {LocalSrcPort, OwnDev} or {Orig-SrcPort, SrcDev}
                                                L2MLL - Used for source filtering*/
        GT_U32                  virtualSrcDev;/*<VirtualSrcPort> together with <VirtualSrcDev> are used for Bridging and
                                                learning instead of {LocalSrcPort, OwnDev} or {Orig-SrcPort, SrcDev}
                                                L2MLL - Used for source filtering*/
    }bridgeToMllInfo;/* special info between L2I and L2MLL */

    GT_U32  egressPassangerTagTpidIndex;/* set by : EQ -  the Logical port map table
                                           used by: HA - build passenger tag0,1 .

                                    Index to the Egress Logical Port TPID table,
                                    where each entry defines a Tag0 TPID and Tag1 TPID.
                                    NOTE: This configuration does not apply to the
                                         Tunnel Start MAC header VLAN tag TPID,
                                         which has a per physical port TPID configuration. */

    GT_U32  numberOfSubscribers;  /* number of subscribers / references to this descriptor */

    GT_BIT   isFcoe;              /* Is packet FCoE (ethertype == 0x8906) */
    struct{ /* fcoeInfo */
        GT_BIT   fcoeLegal;       /* Indicates that FCoE packet is correct */
        GT_BIT   fcoeFwdEn;       /* Indicates that FCoE forwarding enabled */
    }fcoeInfo;

    CHT_PACKET_CLASSIFICATION_TYPE_ENT pcktType; /* packet type, used in TTI and PCL */
    SIP5_PACKET_CLASSIFICATION_TYPE_ENT tti_pcktType_sip5; /* sip 5 packet type, used in TTI */
    SIP5_PACKET_CLASSIFICATION_TYPE_ENT pcl_pcktType_sip5; /* sip 5 packet type, used in PCL */
    SIP5_PACKET_CLASSIFICATION_TYPE_ENT epcl_pcktType_sip5;/* sip5: <egressUdbPacketType> Packet Type used for indexing the HA UDB configuration table
                                                            and EKGEN UDB selection table.
                                                            Set by TTI according to the Packet Type and HA requirements
                                                            (i.e. ignoring TTI <Enable parsing of passenger over xxx> configurations)*/

    GT_BIT  isMultiTargetReplication;    /* is packet multi target and need to replicate */
    GT_BIT  isMultiTarget;    /* is packet multi target
                        same value as trillMcDescriptorInstance but for general support (not only trill))*/

    SNET_INGRESS_DSA_STC ingressDsa;

    GT_U32  ipTTKeyProtocol;  /* ip tunneling protocol, used in TTI only */
    GT_U32  pclId;  /* pcl id, used in TTI only */

    GT_BIT rxEnableProtectionSwitching;/*sip5:
                                  0 - Disable Rx Protection Switching
                                  1- Enable Rx Protection Switching*/
    GT_BIT rxIsProtectionPath;  /*sip5: relevant when rxEnableProtectionSwitching = 1
                                  0 - Packet arrived on Working Path
                                  1 - Packet arrived on Protection Path*/
    GT_BIT channelTypeToOpcodeMappingEn;/*Init: TTI.Terminated: IPCL.
                                    Assigned by the TTI action. Indicates whether the MPLS G-ACh Channel
                                    Type should be mapped to an OAM Opcode using the Channel Type to
                                    Opcode mapping configuration*/
    GT_U32 channelTypeProfile;/* the Channel Type Profile used for build the TTI key for MPLS lookup*/
    GT_U32  ipclUdbConfigurationTableUdeIndex;/* SIP5:
                                                Direct index to one of 7 UDB configuration table user defined ethertype entries,
                                                from index 9 to 15.
                                                Used for extracting packet header fields for packets that differ in their header
                                                format, but share the same ethertype (e.g. MPLS and MPLS-TP)
                                                0 - Disabled: Use packet ethertype to select the UDB configuration table entry
                                                1- Assign UDB configuration table index 9
                                                2 - Assign UDB configuration table index 10
                                                .....
                                                7 - Assign UDB configuration table index 15*/
    GT_BIT enableL3L4ParsingOverMpls;
    GT_U32 ipxLength;       /* ip total length in bytes (header + payload) */
    GT_U32 ipxHeaderLength; /* The number of 32 bit words in the IP header. For IPv4 it is
                                taken from the IPv4 header field IHL. For IPv6 it is valid only when
                                Desc<L4 valid> = 1. It is used by the bridge in L4 security checks. */

    SKERNEL_EGR_PACKET_TYPE_ENT     egressPacketType;/* egress packet type */
    SKERNEL_INNER_PACKET_TYPE_ENT   innerPacketType;/* inner packet type .
                                                       relevant when TT = 1

                                                       values are :
                                                        0 - ipv4
                                                        1 - mpls
                                                        2 - eth + crc
                                                        3 - eth
                                                        4 - unknown
                                                       */
    GT_BIT                             isMplsLsr;     /* xcat and above : is this MPLS LSR */
    GT_U32                             mplsLsrOffset; /* xcat and above : offset due to LSR label operations */

    SNET_EGRESS_PHYSICAL_PORT_INFO_STC egressPhysicalPortInfo;

    GT_BOOL                      useArpForMacSa;/* SIP5 : indication that the HA unit need to use 'ARP pointer' to get the mac SA !!! */
    GT_U8                        newMacSa[6];/* SIP5 : mac SA assigned by the PCL action 'Replace MAC SA'*/
    GT_U8                        *origMacSaPtr;/* pointer to the mac SA in the packet before PCL action did 'Replace MAC SA' */

    GT_BIT                      ttiMetadataReady;/* sip5 : is the tti metadata ready ?
                                                0 - not ready .
                                                1 - ready
                                                when the metadata needed and ttiMetadataReady == 0 , then ttiMetadataInfo[] is filled
                                                */
    GT_U32                      ttiMetadataInfo[ (256+31) / 32];/* sip5 : supporting 256 bits of metadata in sip5
                                                the simulation will build the 256 bits of metadata , and the 'build key' will use the needed bytes from it.
                                                */
    GT_BIT                      ipclMetadataReady;/* sip5 : is the ipcl metadata ready ?
                                                0 - not ready .
                                                1 - ready
                                                when the metadata needed and ipclMetadataInfo == 0 , then ipclMetadataInfo[] is filled
                                                */
    GT_U32                      ipclMetadataInfo[ (792+31) / 32];/* sip5 : supporting 792 bits of metadata in sip5
                                                the simulation will build the 792 bits of metadata , and the 'build key' will use the needed bytes from it.
                                                */

    GT_BIT                      epclMetadataReady;/* sip5 : is the epcl metadata ready ?
                                                0 - not ready .
                                                1 - ready
                                                when the metadata needed and epclMetadataInfo == 0 , then epclMetadataInfo[] is filled
                                                */
    GT_U32                      epclMetadataInfo[ (728+31) / 32];/* sip5 : supporting 728 bits of metadata in sip5
                                                the simulation will build the 728 bits of metadata , and the 'build key' will use the needed bytes from it.
                                                */

    struct CHT_PCL_EXTRA_PACKET_INFO_STCT   *pclExtraDataPtr;/* pointer to PCL extra info */


    GT_U32                      firstVlanTagExtendedSize; /* sip5 only, and "support802_1br_PortExtender" : indicates whether first vlan tag is extended or not . (when DSA port this is tag after DSA)
                                            value 0 means - not extended (0 bytes)
                                            value 4 means - not extended (4 bytes)
                                            value 6 means - extended (6 bytes)
                                            value 8 means - extended (8 bytes)
                                            ===== special cases =======
                                            value 10 means - 6+4 = 10 bytes --> support errata. (sip5)
                                            value 12 means - 8+4 = 12 bytes --> support errata. (sip5)
                                            */

    GT_U32                      tmQueueId; /* sip5 : traffic manager queue ID to for packet enqueue */
    GT_U32                      copyReserved; /* sip5 : reserved descriptor bits for simplifying design changes for new requirments */
    GT_U32                      greHeaderSize; /* num of bytes for the GRE header : 0 / 4 / 8 */

    GT_U32                      tcpFlags;/*6 bits of TCP flags */
    GT_BIT                      tcpSyn;/* TCP with syn flag */
    GT_BIT                      tcpSynWithData;/* TCP with syn flag and contain payload */
    GT_BIT                      ecnCapable; /* valid for FORWARD/FROM_CPU packets
                                    indicates that the packet support can mark the ipv4/6<ECN> field of this packet*/
    GT_BIT                      qcnRx;/* indicates if the received packet is a QCN packet as identified by the ethertype */
    GT_U32                      origRxQcnPrio; /* valid when qcnRx = 1 , this field contains the priority of the packet that originally triggered the QCN packet */
    SKERNEL_EGRESS_PACKET_TYPE_ENT  outerPacketType;/* sip5 egress packet type */

    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_STATE_ENT  tr101EgressVlanTagMode;/*tr101 egress vlan tag mode */
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_STATE_ENT  passangerTr101EgressVlanTagMode;/*in case of TS this is tr101 egress vlan tag mode of passenger */

    GT_U32                      queue_dp; /* used in TXQ to know the drop precedence. It is used instead of dp!!!, because dp is field that used down the pipe also,
                                             and it may differ from this value. This field set by the EGF (qag) and used by the TXQ (needed for {tc,dp}
                                             remap to {tc,dp} for control traffic and traffic to cascade) */
    GT_U32                      queue_priority; /*used in TXQ to know the egress queue. It is used instead of tc!!!, because tc is field that used down the pipe also,
                                                 and it may differ from this value. This field set by the EGF (qag) and used by the TXQ (needed for {tc,dp}
                                                 remap to {tc,dp} for control traffic and traffic to cascade) */
    GT_BIT                      isEqCncOrigCounted;/*indication that current descriptor from EQ unit did CNC clients of 'ingress vlan pass/drop' and 'packet type pass/drop' */
    GT_U16                      txqDestPorts[SKERNEL_CHEETAH_EGRESS_MAX_PORT_CNS];/* sip5 only : the egress physical ports in EGF_QAG stage that are mapped to TXQ_PORTs(0..71) ,
                                            see also bmpOfHemisphereMapperPtr */
    SKERNEL_PORTS_BMP_STC *bmpOfHemisphereMapperPtr;/* SIP5 : pointer to bmpOfHemisphereMapper */
    SKERNEL_PORTS_BMP_STC bmpOfHemisphereMapper;/*IP5 : (pointer to) bmp of mapping target port to hemisphere*/


    GT_U32      bmpsEqUnitsGotTxqMirror ;/* bmp of EQ units that got mirror:
                                        Lion3 : Egress mirror sent to egress core, this was done to reduce design complexity.
                                        Lion1/2 : Egress mirror sent to ingress core.
                                        */
    GT_BOOL     isCpuUseSdma;      /* when sent to CPU , is it using SDMA mechanism ? */
    SKERNEL_DEVICE_OBJECT * cpuPortGroupDevObjPtr;/* the 'CPU' device object that
                    represents the 'egress portGroup' */

    GT_U32      trafficManagerEnabled;/* indication that this descriptor should go to the traffic manager */
    GT_U32      trafficManagerTc;/* traffic class for traffic manager */
    GT_U32      trafficManagerFinalPort;/* traffic manager final egress port decision */
    GT_U32      trafficManagerCos;/* traffic manager cos */
    GT_U32      trafficManagerColor;/* traffic manager color (like dp)*/
    GT_U32      inDsaPktOrigBC;     /* Input DSA field <Pkt Orig BC>, valid for input DSA tagged packet with TO_CPU DSA */
    GT_U32      bpe802_1br_GRP; /*hold the <GRP> field from ETag according to IEEE 802.1br .
                                                used ONLY inside TTI unit !!! (other units should no relay on it !!!)
                                                Part of xCat3 support */

}SKERNEL_FRAME_CHEETAH_DESCR_STC;


/*
   assign into 'Qos field' the value from the descriptor :
    get the 'Qos field' form the descriptor , note that the MACRO may get
   the 'Qos field' value from the 'TRILL info'

   -- to support 'Inner frame' of TRILL for 'QoS trust' mode

   NOTE: when the TTI key did lookup match , the 'Qos field' taken from the descriptor
   */
#define SKERNEL_CHT_DESC_INNER_FRAME_QOS_FIELD_GET_MAC(_descPtr , _field)   \
    if(_descPtr->ttiLookupMatch == 1 && \
        (_descPtr->ingressTunnelInfo.transitType != SNET_INGRESS_TUNNEL_TRANSIT_TYPE__NONE__E))  \
        _field = _descPtr->_field ;    \
    else                               \
        SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(_descPtr , _field)

/*
   assign into field the value from the descriptor :
    get the field form the descriptor , note that the MACRO may get
   the value from the 'TRILL info'

   -- to support 'Inner frame' of TRILL

   NOTE: when the TTI key did lookup match , the fields taken from the descriptor
   */
#define SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(_descPtr , _field)   \
    _field =                                                \
    ((_descPtr->ingressTunnelInfo.innerFrameDescrPtr) ?             \
     (_descPtr->ingressTunnelInfo.innerFrameDescrPtr)->_field :    \
     _descPtr->_field)



/* Out of profile traffic */
typedef enum {
    SKERNEL_CONFORM_GREEN = 0,
    SKERNEL_CONFORM_YELLOW,
    SKERNEL_CONFORM_RED,
    SKERNEL_CONFORM_DROP
}SKERNEL_CONFORMANCE_LEVEL_ENT;

typedef enum{
    SKERNEL_TIME_UNITS_NOT_VALID_E,
    SKERNEL_TIME_UNITS_MILI_SECONDS_E,  /*10e-3*/
    SKERNEL_TIME_UNITS_MICRO_SECONDS_E, /*10e-6*/
    SKERNEL_TIME_UNITS_NANO_SECONDS_E,  /*10e-9*/
    SKERNEL_TIME_UNITS_PICO_SECONDS_E,  /*10e-12*/

    SKERNEL_TIME_UNITS__LAST__E
}SKERNEL_TIME_UNITS_ENT;

typedef struct{
    SKERNEL_TIME_UNITS_ENT  timeUnit;/* type of time set in timeBetweenPackets */

    GT_U32   timeBetweenPackets;/* sip 5 : allow feature like policer ability to
                                   update the token buckets according to 'time' passed
                                   from last packet that hit the meter entry */
}SKERNEL_TIME_UNIT_STC;

typedef struct{
    GT_BOOL disableFatalError;
    SKERNEL_CONFORMANCE_LEVEL_ENT   policerConformanceLevel;
    SKERNEL_TIME_UNIT_STC           enhancedPolicerIfgTime;/* policer inter-frame gap */
    GT_BOOL  printGeneralAllowed;   /* do we allow simulationPrintf for general info :
                                       default is 'no' , may be changed from INI file  */
    GT_U32   printWarningAllowed;   /* do we allow simulationPrintf for warning info :
                                       default is 'yes' , may be changed from INI file  */
}SKERNEL_USER_DEBUG_INFO_STC;

/* User debug info */
extern SKERNEL_USER_DEBUG_INFO_STC skernelUserDebugInfo;

/* use simGeneralPrintf instead of 'simulationPrintf' for general info printing */
#define simGeneralPrintf   \
    if(skernelUserDebugInfo.printGeneralAllowed) simulationPrintf

/* use simWarningPrintf instead of 'simulationPrintf' for warnings info printing */
#define simWarningPrintf   \
    if(skernelUserDebugInfo.printWarningAllowed) simulationPrintf

/* use simForcePrintf instead of 'simulationPrintf' to force printing */
#define simForcePrintf      simulationPrintf

/**
 *  Union : SNET_DST_INTERFACE_UNT
 *
 *  Description : generic destination interface union.
 *
*/
typedef union{
    struct{
        GT_U32   port;
        GT_U32   devNum;
    }devPort;

    GT_U16  trunkId;
    GT_U16  vidx;
}SNET_DST_INTERFACE_UNT;

/**
 *  Enum : SNET_DST_INTERFACE_ENT
 *
 *  Description : generic destination interface enumerator (Port/Trunk/Vidx).
*/
typedef enum{
    SNET_DST_INTERFACE_PORT_E,
    SNET_DST_INTERFACE_TRUNK_E,
    SNET_DST_INTERFACE_VIDX_E
}SNET_DST_INTERFACE_ENT;

/**
 *  Structure : SNET_DST_INTERFACE_STC
 *
 *  Description : generic destination interface structure.
 *
*/
typedef struct{
    SNET_DST_INTERFACE_ENT  dstInterface;
    SNET_DST_INTERFACE_UNT  interfaceInfo;
}SNET_DST_INTERFACE_STC;

#define SNET_SET_DEST_INTERFACE_TYPE(interface, type, en) \
    if (en) { \
        interface.dstInterface = type; \
    }

#define SNET_GET_DEST_INTERFACE_TYPE(interface, type) \
    (interface.dstInterface == type) ? 1 : 0

/* Apply destination interface data on the relevant descriptor fields */
#define SNET_APPLY_DESTINATION_INTERFACE_MAC(interface, descr) \
    if (SNET_GET_DEST_INTERFACE_TYPE((interface), SNET_DST_INTERFACE_VIDX_E)) { \
        (descr)->eVidx = (interface).interfaceInfo.vidx; \
    } else if (SNET_GET_DEST_INTERFACE_TYPE((interface), SNET_DST_INTERFACE_TRUNK_E)) { \
        (descr)->trgTrunkId = (interface).interfaceInfo.trunkId; \
    } else { \
        (descr)->trgEPort = (interface).interfaceInfo.devPort.port; \
        (descr)->trgDev = (interface).interfaceInfo.devPort.devNum; \
    }

/**
 *  Union : SNET_EXT_DST_INTERFACE_UNT
 *
 *  Description :  extended generic destination interface union.
 *
*/
typedef union{
    SNET_DST_INTERFACE_UNT egressInterface;
    GT_U32  routerLLTIndex;
    GT_U32  virtualRouterId;
}SNET_EXT_DST_INTERFACE_UNT;


/**
 *  Enum : SNET_EXT_DST_INTERFACE_ENT
 *
 *  Description : extended generic destination interface enumerator (Egress Interface/RouterLLT Index/VRF-ID).
*/
typedef enum{
    SNET_DST_INTERFACE_EGRESS_E,
    SNET_DST_INTERFACE_LLT_INDEX_E,
    SNET_DST_INTERFACE_VIRTUAL_ROUTER_E
}SNET_EXT_DST_INTERFACE_ENT;

/**
 *  Structure : SNET_EXT_DST_INTERFACE_STC
 *
 *  Description : extended generic destination interface structure.
 *
*/
typedef struct{
    SNET_EXT_DST_INTERFACE_ENT  dstInterface;
    SNET_EXT_DST_INTERFACE_UNT  interfaceInfo;
}SNET_EXT_DST_INTERFACE_STC;

#define ADDRESS_COMPLETION_STATUS_GET_MAC(devPtr) (((devPtr)->gmDeviceType == GOLDEN_MODEL) ? 1 : 0)
#define IS_NIC_DEVICE_MAC(devPtr) (((devPtr)->deviceFamily == SKERNEL_NIC_FAMILY) ? 1 : 0)


/* SKERNEL_BITWISE_OPERATOR_ENT :
    enum for bitwise operators

    to allow actions of :
    x = (~A) ^ (B)
    x = (~A) | (~B)

    see function skernelBitwiseOperator
*/
typedef enum {
    SKERNEL_BITWISE_OPERATOR_NONE_E ,/* no operation */

    SKERNEL_BITWISE_OPERATOR_XOR_E , /* XOR (^) operation */
    SKERNEL_BITWISE_OPERATOR_AND_E , /* AND (&) operation */
    SKERNEL_BITWISE_OPERATOR_OR_E  , /* OR (|) operation */

    SKERNEL_BITWISE_OPERATOR_NOT_E = 0x1000, /* NOT (~) operation */

}SKERNEL_BITWISE_OPERATOR_ENT;

/*******************************************************************************
*   skernelBitwiseOperator
*
* DESCRIPTION:
*       calculate bitwise operator result:
*       result =  (((value1Operator) on value1) value2Operator on value2)
*
* INPUTS:
*        value1  - value 1
*        value1Operator - operator to be done on value 1
*                       valid are:
*                       SKERNEL_BITWISE_OPERATOR_NONE_E
*                       SKERNEL_BITWISE_OPERATOR_NOT_E  (~value1)
*        value2  - value 2
*        value2Operator - operator to be done on value 1
*                       valid are:
*                       SKERNEL_BITWISE_OPERATOR_NONE_E -> (value 2 ignored !)
*                       SKERNEL_BITWISE_OPERATOR_XOR_E  -> ^ value2
*                       SKERNEL_BITWISE_OPERATOR_AND_E  -> & value2
*                       SKERNEL_BITWISE_OPERATOR_OR_E   -> | value2
*                       SKERNEL_BITWISE_OPERATOR_NOT_E  -> this is 'added' to on of 'xor/or/and'
*                                                       -> ^ (~vlaue2)
*                                                       -> & (~vlaue2)
*                                                       -> | (~vlaue2)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       the operator result
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32  skernelBitwiseOperator
(
    IN GT_U32                       value1,
    IN SKERNEL_BITWISE_OPERATOR_ENT value1Operator,

    IN GT_U32                       value2,
    IN SKERNEL_BITWISE_OPERATOR_ENT value2Operator
);


/*******************************************************************************
*   skernelIsCheetahDev
*
* DESCRIPTION:
*       check if device should behave like cheetah
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch device
*       1 - ch device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsCheetahDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);


/*******************************************************************************
*   skernelIsCheetah1OnlyDev
*
* DESCRIPTION:
*       check if device should behave like cheetah and not ch2,3...
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch device
*       1 - ch device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsCheetah1OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);


/*******************************************************************************
*   skernelIsCheetah2OnlyDev
*
* DESCRIPTION:
*       check if device should behave like cheetah 2  and not ch,ch3..
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch2 device
*       1 - ch2 device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsCheetah2OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsCheetah3OnlyDev
*
* DESCRIPTION:
*       check if device should behave like cheetah 3 (or ch3+) and not xcat..
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch3 (or ch3+) device
*       1 - ch3 device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsCheetah3OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsXcatOnlyDev
*
* DESCRIPTION:
*       check if device should behave like xCat and not others..
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xcat device
*       1 - xcat device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsXcatOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsXcat2OnlyDev
*
* DESCRIPTION:
*       Check if device should behave like xCat2 and not others.
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xCat2 device
*       1 - xCat2 device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsXcat2OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsXcat3OnlyDev
*
* DESCRIPTION:
*       Check if device should behave like xCat3 and not others.
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xCat3 device
*       1 - xCat3 device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsXcat3OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsLionPortGroupOnlyDev
*
* DESCRIPTION:
*       check if device should behave like Lion's port group
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion's port group device
*       1 - Lion's port group device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsLionPortGroupOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsLionPortGroupDev
*
* DESCRIPTION:
*       check if device should behave like Lion shell
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion shell device
*       1 - Lion shell device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsLionPortGroupDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsCheetah3Dev
*
* DESCRIPTION:
*       check if device should behave like cheetah 3
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch3 device
*       1 - ch3 device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsCheetah3Dev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsCheetah3OnlyXgDev
*
* DESCRIPTION:
*       check if device cheetah 3 XG (all ports are XG) -- not supports future
*               devices.
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch3 device
*       1 - ch3 device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsCheetah3OnlyXgDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsXcatDev
*
* DESCRIPTION:
*       check if device should behave like xCat
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xCat device
*       1 - xCat device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsXcatDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsXcat2Dev
*
* DESCRIPTION:
*       Check whether device should behave like xCat2
*
* INPUTS:
*        devObjPtr  - pointer to device object
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xCat2 device
*       1 - xCat2 device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsXcat2Dev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsPhyShellOnlyDev
*
* DESCRIPTION:
*       check if device should behave like PHY shell
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not PHY shell device
*       1 - PHY shell device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsPhyShellOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);
/*******************************************************************************
*   skernelIsPhyOnlyDev
*
* DESCRIPTION:
*       check if device should behave like PHY core
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not PHY core device
*       1 - PHY core device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsPhyOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsMacSecOnlyDev
*
* DESCRIPTION:
*       check if device should behave like macSec
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not macSec device
*       1 - macSec device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsMacSecOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsLion2PortGroupOnlyDev
*
* DESCRIPTION:
*       check if device should behave like Lion2's port group
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion's port group device
*       1 - Lion's port group device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsLion2PortGroupOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsLion2PortGroupDev
*
* DESCRIPTION:
*       check if device should behave like Lion shell
*
* INPUTS:
*        deviceType  - device type
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion port group device
*       1 - Lion port geoup device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelIsLion2PortGroupDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   skernelIsLion3PortGroupOnlyDev
*
* DESCRIPTION:
*       check if device should behave like Lion3's port group
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion3's port group device
*       1 - Lion3's port group device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsLion3PortGroupOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);

/*******************************************************************************
*   smainMemAddrDefaultGet
*
* DESCRIPTION:
*       Load default values for memory for specified address from text file
*
* INPUTS:
*       deviceObjPtr - pointer to the device object.
*       fileNamePtr - default values file name.
*       address     - lookup address.
*
* OUTPUTS:
*       valuePtr        - pointer to lookup address value
*
* RETURNS:
*
*       GT_OK       - lookup address found
*       GT_FAIL     - lookup address not found
*       GT_BAD_PTR  - NULL pointer
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS smainMemAddrDefaultGet (
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_CHAR                  * fileNamePtr,
    IN GT_U32                   address,
    OUT GT_U32                  * valuePtr
);

/*******************************************************************************
*   smainMemAdditionalAddrDefaultGet
*
* DESCRIPTION:
*       Load default values for memory for specified address from additional text file
*
* INPUTS:
*       deviceObjPtr - pointer to the device object.
*       devId        - the deviceId to use when build the name in INI file to look
*                       for
*       address     - lookup address.
*
* OUTPUTS:
*       valuePtr        - pointer to lookup address value
*
* RETURNS:
*
*       GT_OK       - lookup address found
*       GT_FAIL     - lookup address not found
*       GT_BAD_PTR  - NULL pointer
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS smainMemAdditionalAddrDefaultGet
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_U32                   devId,
    IN GT_U32                   address,
    OUT GT_U32                  * valuePtr
);

/*******************************************************************************
*   smainMemDefaultsLoadAll
*
* DESCRIPTION:
*       Load default values for device memory from all default registers files
*
* INPUTS:
*       deviceObjPtr    - pointer to the device object
*       devId           - device Id
*       sectionPtr      - pointer to section name
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smainMemDefaultsLoadAll
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_U32                   devId,
    IN GT_CHAR                  * sectionPtr
);

/*******************************************************************************
*   smainBusDevFind
*
* DESCRIPTION:
*       find device on the bus.
*
* INPUTS:
*       busId - the device that we look the device on
*       targetHwDeviceId - the target HW device ID of the device that we look for.
*
* OUTPUTS:
*       targetDeviceIdPtr - (pointer to) the target device number.
*                       with this number we can access the SCIB API to access
*                       this device.
*
* RETURNS:
*       GT_TRUE - device found
*       GT_FALSE - device not found
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL smainBusDevFind
(
    IN GT_U32   busId,
    IN GT_U32   targetHwDeviceId,
    OUT GT_U32  *targetDeviceIdPtr
);

/*******************************************************************************
*   GENERIC_MSG_FUNC
*
* DESCRIPTION:
*       typedef for generic function to handle any message.
*       The function gets the pointer to the buffer where the data encapsulated ,
*       and the length of message.
*
*       NOTE:
*       1. With this technique no need to define to many values in enum of SMAIN_MSG_TYPE_ENT
*          so there is no need to add 'binding' like done for :
*            SNET_DEV_LINK_UPD_PROC_FUN  devPortLinkUpdateFuncPtr;
*            SFDB_DEV_MSG_PROC_FUN       devFdbMsgProcFuncPtr;
*            ...
*            SNET_DEV_FROM_EMBEDDED_CPU_PROC_FUN devFromEmbeddedCpuFuncPtr;
*       2. the mechanism requires to put the CB function on the message itself as first parameters ,
*          but the SMAIN parser will REMOVE it from the message before calling the CB with
*           dataPtr , dataLength.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       dataPtr     - pointer to message info.
*       dataLength  - length of the message (in bytes).
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
typedef void (*GENERIC_MSG_FUNC)(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U8                 * dataPtr,
    IN GT_U32                  dataLength
);

/*******************************************************************************
*   skernelPortLoopbackForceModeSet
*
* DESCRIPTION:
*       the function set the 'loopback force mode' on a port of device.
*       this function needed for devices that not support loopback on the ports
*       and need 'external support'
*
* INPUTS:
*       deviceId  - the simulation device Id .
*       portNum   - the physical port number .
*       mode      - the loopback force mode.
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM     - on bad portNum or mode
*
* COMMENTS:
*       function do fatal error on non-exists device or out of range device.
*
*******************************************************************************/
GT_STATUS skernelPortLoopbackForceModeSet
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN SKERNEL_PORT_LOOPBACK_FORCE_MODE_ENT mode
);

/*******************************************************************************
*   skernelPortLinkStateSet
*
* DESCRIPTION:
*       the function set the 'link state' on a port of device.
*       this function needed for devices that not support 'link change' from the
*       'MAC registers' of the ports.
*       this is relevant to 'GM devices'
*
* INPUTS:
*       deviceId  - the simulation device Id .
*       portNum   - the physical port number .
*       linkState - the link state to set for the port.
*                   GT_TRUE  - force 'link UP'
*                   GT_FALSE - force 'link down'
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM     - on bad portNum or mode
*
* COMMENTS:
*       function do fatal error on non-exists device or out of range device.
*
*******************************************************************************/
GT_STATUS skernelPortLinkStateSet
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN GT_BOOL                      linkState
);

/*******************************************************************************
*   skernelUnbindDevPort2Slan
*
* DESCRIPTION:
*       The function unbinds given dev and port from slan
*
* INPUTS:
*       deviceId     - the simulation device Id
*       portNum         - the physical port number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId or portNum
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelUnbindDevPort2Slan
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum
);

/*******************************************************************************
* skernelBindDevPort2Slan
*
* DESCRIPTION:
*       The function binds given dev and port to slan
*
* INPUTS:
*       deviceId     - the simulation device Id
*       portNum         - the physical port number
*       slanNamePtr  - slan name string,
*       unbindOtherPortsOnThisSlan -
*                      GT_TRUE  - if the given slanName is bound to any other port(s)
*                                 unbind it from this port(s) before bind it to the new port.
*                                 This will cause link change (link down) on this other port(s).
*                      GT_FALSE - bind the port with slanName regardless
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId, portNum, slan name
*       GT_FAIL       - on error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelBindDevPort2Slan
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN  GT_CHAR                    *slanNamePtr,
    IN  GT_BOOL                     unbindOtherPortsOnThisSlan
);

/*******************************************************************************
* skernelDevPortSlanGet
*
* DESCRIPTION:
*       The function get the slanName of a port of device
*
* INPUTS:
*       deviceId     - the simulation device Id
*       portNum         - the physical port number
*       slanMaxLength - the length of the buffer (for the string) that is
*                       allocated by the caller for slanNamePtr
*                       when slanNamePtr != NULL then slanMaxLength must be >= 8
*
* OUTPUTS:
*       portBoundPtr - (pointer to)is port bound.
*       slanNamePtr  - (pointer to)slan name (string).
*                      The caller function must cares about memory allocation.
*                      NOTE:
*                      1. this parameter can be NULL ... meaning that caller not
*                         care about the slaqn name only need to know that port
*                         bound / not bound
*                      2. if slanMaxLength is less then the actual length of
*                         the slanName then only part of the name will be returned.
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId or portNum.
*                       when slanNamePtr!=NULL but slanMaxLength < 8
*       GT_BAD_PTR    - on NULL pointer portBoundPtr
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelDevPortSlanGet
(
    IN  GT_U32                deviceId,
    IN  GT_U32                portNum,
    IN  GT_U32                slanMaxLength,
    OUT GT_BOOL              *portBoundPtr,
    OUT GT_CHAR              *slanNamePtr
);

/*******************************************************************************
* skernelDevPortSlanPrint
*
* DESCRIPTION:
*       The function print the slanName of a port of device
*
* INPUTS:
*       deviceId     - the simulation device Id
*       portNum         - the physical port number
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId or portNum
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelDevPortSlanPrint
(
    IN  GT_U32                deviceId,
    IN  GT_U32                portNum
);

/*******************************************************************************
* skernelDevSlanPrint
*
* DESCRIPTION:
*       The function print ports slanName of the device
*
* INPUTS:
*       deviceId     - the simulation device Id
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelDevSlanPrint
(
    IN  GT_U32                deviceId
);

/*******************************************************************************
* skernelFatherDeviceIdFromSonDeviceIdGet
*
* DESCRIPTION:
*       The function convert 'son device id' to 'father deviceId'.
*       for device with no 'father' --> return 'son deviceId'
*
* INPUTS:
*       sonDeviceId     - the simulation device Id of the 'son'
*
* OUTPUTS:
*       fatherDeviceIdPtr - (pointer to)the simulation device Id of the 'father'
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad sonDeviceId .
*       GT_BAD_PTR    - on NULL pointer
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelFatherDeviceIdFromSonDeviceIdGet
(
    IN  GT_U32                sonDeviceId,
    OUT GT_U32               *fatherDeviceIdPtr
);

/*******************************************************************************
*   smainMemDefaultsLoad
*
* DESCRIPTION:
*       Load default values for memory from file
*
* INPUTS:
*       deviceObjPtr - pointer to the device object.
*       fileNamePtr  - default values file name.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainMemDefaultsLoad
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_CHAR                  * fileNamePtr
);

/*******************************************************************************
*   smainMemConfigLoad
*
* DESCRIPTION:
*       Load memory configuration from file
*
* INPUTS:
*       deviceObjPtr - pointer to the device object.
*       fileNamePtr  - default values file name.
*       isSmemUsed   - whether to use smem func instead of scib
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainMemConfigLoad
(
    IN SKERNEL_DEVICE_OBJECT    *deviceObjPtr,
    IN GT_CHAR                  *fileNamePtr,
    IN GT_BOOL                   isSmemUsed
);

/*******************************************************************************
*  smainDeviceBackUpMemoryTest
*
* DESCRIPTION:
*      Test backup memory function
*
* INPUTS:
*       deviceNumber - device ID.
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - success
*       GT_FAIL - wrong device ID
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS smainDeviceBackUpMemoryTest
(
    IN  GT_U8    deviceNumber
);

/*******************************************************************************
*   smainPacketGeneratorTask
*
* DESCRIPTION:
*       Packet generator main task
*
* INPUTS:
*       tgDataPtr - pointer to the traffic generator data.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID smainPacketGeneratorTask
(
    IN SKERNEL_TRAFFIC_GEN_STC * tgDataPtr
);

/*******************************************************************************
*   skernelCreatePacketGenerator
*
* DESCRIPTION:
*       Create packet generator task
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       tgNumber     - packet generator number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID skernelCreatePacketGenerator
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 tgNumber
);

/*******************************************************************************
*   skernelNumOfPacketsInTheSystemSet
*
* DESCRIPTION:
*       increment/decrement the number of packet in the system
*
* INPUTS:
*       increment - GT_TRUE  - increment the number of packet in the system
*                   GT_FALSE - decrement the number of packet in the system
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID skernelNumOfPacketsInTheSystemSet(
    IN GT_BOOL     increment
);

/*******************************************************************************
*   skernelNumOfPacketsInTheSystemGet
*
* DESCRIPTION:
*       Get the number of packet in the system
*
* INPUTS:
*       None.
* OUTPUTS:
*       None.
*
* RETURNS:
*       The number of packet in the system
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelNumOfPacketsInTheSystemGet(GT_VOID);

/*******************************************************************************
*   skernelForceLinkDownOrLinkUpOnAllPorts
*
* DESCRIPTION:
*       force link UP/DOWN on all ports
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       forceLinkUp -  GT_TRUE  - force link UP   on all ports
*                      GT_FALSE - force link DOWN on all ports
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void skernelForceLinkDownOrLinkUpOnAllPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_BOOL                 forceLinkUpOrLinkDown
);

/*******************************************************************************
*   skernelDeviceSoftResetGeneric
*
* DESCRIPTION:
*       do generic soft reset.
*       Function do soft reset on the following order:
*           - force link of all active SLANs down
*           - disable device interrupts
*           - unbind SCIB interface
*           - create new device to replace old one
*             the new device will keep the threads of the old one + queues + pools
*             the create of new device will reopen the SLANs , interrupt line , SCIB bind...
*           - delete old device object (with all it's memories)
*
* INPUTS:
*       oldDeviceObjPtr - pointer to the old device
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to the new allocated device.
* COMMENTS:
*       called in context of skernel task
*
*       see function smemLion3ActiveWriteDfxServerResetControlReg that triggers
*       soft reset
*******************************************************************************/
SKERNEL_DEVICE_OBJECT * skernelDeviceSoftResetGeneric
(
    IN SKERNEL_DEVICE_OBJECT * oldDeviceObjPtr
);

/*******************************************************************************
*   skernelDeviceSoftResetGenericPart2
*
* DESCRIPTION:
*       part 2 of do generic soft reset.
*       resume pool and queue of the device to empty state.
*       and state newDeviceObjPtr->softResetOldDevicePtr = NULL
*
* INPUTS:
*       newDeviceObjPtr - pointer to the new device !!!
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       !!! NULL !!!
* COMMENTS:
*       called in context of skernel task
*
*       this is part 2 of skernelDeviceSoftResetGeneric
*******************************************************************************/
SKERNEL_DEVICE_OBJECT * skernelDeviceSoftResetGenericPart2
(
    IN SKERNEL_DEVICE_OBJECT * newDeviceObjPtr
);


/*******************************************************************************
* skernelSleep
*
* DESCRIPTION:
*       Puts current task to sleep for specified number of millisecond.
*       this function needed instead of direct call to SIM_OS_MAC(simOsSleep)
*       because it allow the thread that calls this function to replace the 'old device'
*       with 'new device' as part of 'soft reset' processing.
*
* INPUTS:
*       devObjPtr      - the device that needs the sleep
*       timeInMilliSec - time to sleep in milliseconds
*
* OUTPUTS:
*       None
*
* RETURNS:
*       pointer to device object as the device object may have been replaces due
*       to 'soft reset' at this time
*
* COMMENTS:
*       None
*
*******************************************************************************/
SKERNEL_DEVICE_OBJECT  * skernelSleep
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U32                   timeInMilliSec
);

/*******************************************************************************
*   smainEmulateRemoteTm
*
* DESCRIPTION:
*       debug tool to be able to emulte remote TM for basic testing of
*       simulation that works with remote TM (udp sockets and asynchronous
*       messages).
*
* INPUTS:
*       simDeviceId     - Simulation device ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainEmulateRemoteTm
(
    IN GT_U32  emulate
);

/*******************************************************************************
*   smainRemoteTmConnect
*
* DESCRIPTION:
*       create the needed sockets for TM device.
*       using UDP sockets (as used by the TM device)
*       and wait for the 'remote TM' to be ready.
*
* INPUTS:
*       simDeviceId     - Simulation device ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainRemoteTmConnect
(
    IN GT_U32   simDeviceId
);

/*******************************************************************************
*   smainGetDevicesString
*
* DESCRIPTION:
*       function to return the names of the devices.
*
* INPUTS:
*       devicesNamesString - (pointer to) the place to set the names of devices
*       sizeOfString    - number of bytes in devicesNamesString.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainGetDevicesString
(
    IN char    *devicesNamesString,
    IN GT_U32   sizeOfString
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smainh */


