/*
 * AdvVCT.h
 *
 *  Created on: 31.10.2014
 *      Author: Tsepelev
 */

#ifndef ADVVCT_H_
#define ADVVCT_H_


typedef struct _SW_VCT_REGISTER
{
    GT_U8    page;
    GT_U8    regOffset;
} SW_VCT_REGISTER;

/* macro to check VCT Failure */
#define IS_VCT_FAILED(_reg)        \
        (((_reg) & 0xFF) == 0xFF)
/* macro to find out if Amplitude is zero */
#define IS_ZERO_AMPLITUDE(_reg)    \
        (((_reg) & 0x7F00) == 0)

/* macro to retrieve Amplitude */
#define GET_AMPLITUDE(_reg)    \
        (((_reg) & 0x7F00) >> 8)

/* macro to find out if Amplitude is positive */
#define IS_POSITIVE_AMPLITUDE(_reg)    \
        (((_reg) & 0x8000) == 0x8000)


#define GT_ADV_VCT_CALC(_data)        \
        (((long)(_data)*8333 - 191667)/10000 + (((((long)(_data)*8333 - 191667)%10000) >= 5000)?1:0))

#define GT_ADV_VCT_CALC_SHORT(_data)        \
        (((long)(_data)*7143 - 71429)/10000 + (((((long)(_data)*7143 - 71429)%10000) >= 5000)?1:0))

#define GT_ADV_VCT_ACCEPTABLE_SHORT_CABLE  11

GT_STATUS simpleVctTest(GT_LPORT port);

GT_STATUS gvctGetAdvCableDiag
(
    GT_LPORT        port,
    GT_ADV_VCT_MODE mode,
    GT_ADV_CABLE_STATUS *cableStatus
);
GT_STATUS advVctTest(GT_LPORT port);
GT_STATUS hwPhyReset
(
    GT_U8        portNum,
    GT_U16        u16Data
);
GT_STATUS getAdvCableStatus_1116
(
    GT_U8           hwPort,
    GT_ADV_VCT_MODE mode,
    GT_ADV_CABLE_STATUS *cableStatus
);
GT_STATUS runAdvCableTest_1116
(
    GT_U8           hwPort,
    GT_BOOL         mode,
    GT_ADV_VCT_TRANS_CHAN_SEL   crosspair,
    GT_ADV_CABLE_STATUS *cableStatus,
    GT_BOOL         *tooShort
);
GT_STATUS runAdvCableTest_1116_get
(
    GT_U8           hwPort,
    GT_ADV_VCT_TRANS_CHAN_SEL    crosspair,
    GT_32            channel,
    GT_ADV_CABLE_STATUS *cableStatus,
    GT_BOOL         *tooShort
);
GT_16 analizeAdvVCTNoCrosspairResult
(
    int     channel,
    GT_U16 *crossChannelReg,
    GT_BOOL isShort,
    GT_ADV_CABLE_STATUS *cableStatus
);
GT_16 analizeAdvVCTResult
(
    int     channel,
    GT_U16 *crossChannelReg,
    GT_BOOL isShort,
    GT_ADV_CABLE_STATUS *cableStatus
);
GT_ADV_VCT_STATUS getDetailedAdvVCTResult
(
    GT_U32  amp,
    GT_U32  len,
    GT_ADV_VCT_STATUS result
);
GT_STATUS runAdvCableTest_1116_check
(
	GT_U8           hwPort
);
GT_STATUS runAdvCableTest_1116_set
(
		GT_U8           hwPort,
		GT_32           channel,
	    GT_ADV_VCT_TRANS_CHAN_SEL        crosspair
);
GT_STATUS driverPagedAccessStop
(
    GT_U8         hwPort,
    GT_U16         pageReg
);
GT_STATUS driverPagedAccessStart
(
    GT_U8         hwPort,
    GT_U16        *pageReg
);
#endif /* ADVVCT_H_ */
