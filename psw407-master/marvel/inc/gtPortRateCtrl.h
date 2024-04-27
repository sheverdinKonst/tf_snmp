#ifndef _gtPortRateCtrl_h
#define _gtPortRateCtrl_h

/*******************************************************************************
* grcSetLimitMode
*
* DESCRIPTION:
*       This routine sets the port's rate control ingress limit mode.
*
* INPUTS:
*       port    - logical port number.
*       mode     - rate control ingress limit mode. 
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*******************************************************************************/
GT_STATUS grcSetLimitMode
(
    IN GT_LPORT          port,
    IN GT_RATE_LIMIT_MODE    mode
);

/*******************************************************************************
* grcGetLimitMode
*
* DESCRIPTION:
*       This routine gets the port's rate control ingress limit mode.
*
* INPUTS:
*       port    - logical port number.
*
* OUTPUTS:
*       mode     - rate control ingress limit mode. 
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
*******************************************************************************/
GT_STATUS grcGetLimitMode
(
    IN  GT_LPORT port,
    OUT GT_RATE_LIMIT_MODE    *mode
);

/*******************************************************************************
* grcSetPri3Rate
*
* DESCRIPTION:
*       This routine sets the ingress data rate limit for priority 3 frames.
*       Priority 3 frames will be discarded after the ingress rate selection
*       is reached or exceeded.
*
* INPUTS:
*       port - the logical port number.
*       mode - the priority 3 frame rate limit mode
*              GT_FALSE: use the same rate as Pri2Rate
*              GT_TRUE:  use twice the rate as Pri2Rate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
*******************************************************************************/
GT_STATUS grcSetPri3Rate
(
    IN GT_LPORT port,
    IN GT_BOOL  mode
);

/*******************************************************************************
* grcGetPri3Rate
*
* DESCRIPTION:
*       This routine gets the ingress data rate limit for priority 3 frames.
*       Priority 3 frames will be discarded after the ingress rate selection
*       is reached or exceeded.
*
* INPUTS:
*       port - the logical port number.
*       
* OUTPUTS:
*       mode - the priority 3 frame rate limit mode
*              GT_FALSE: use the same rate as Pri2Rate
*              GT_TRUE:  use twice the rate as Pri2Rate
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
*******************************************************************************/
GT_STATUS grcGetPri3Rate
(
    IN  GT_LPORT port,
    OUT GT_BOOL  *mode
);

/*******************************************************************************
* grcSetPri2Rate
*
* DESCRIPTION:
*       This routine sets the ingress data rate limit for priority 2 frames.
*       Priority 2 frames will be discarded after the ingress rate selection
*       is reached or exceeded.
*
* INPUTS:
*       port - the logical port number.
*       mode - the priority 2 frame rate limit mode
*              GT_FALSE: use the same rate as Pri1Rate
*              GT_TRUE:  use twice the rate as Pri1Rate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
*******************************************************************************/
GT_STATUS grcSetPri2Rate
(
    IN GT_LPORT port,
    IN GT_BOOL  mode
);

/*******************************************************************************
* grcGetPri2Rate
*
* DESCRIPTION:
*       This routine gets the ingress data rate limit for priority 2 frames.
*       Priority 2 frames will be discarded after the ingress rate selection
*       is reached or exceeded.
*
* INPUTS:
*       port - the logical port number.
*       
* OUTPUTS:
*       mode - the priority 2 frame rate limit mode
*              GT_FALSE: use the same rate as Pri1Rate
*              GT_TRUE:  use twice the rate as Pri1Rate
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
*******************************************************************************/
GT_STATUS grcGetPri2Rate
(
    IN  GT_LPORT port,
    OUT GT_BOOL  *mode
);

/*******************************************************************************
* grcSetPri1Rate
*
* DESCRIPTION:
*       This routine sets the ingress data rate limit for priority 1 frames.
*       Priority 1 frames will be discarded after the ingress rate selection
*       is reached or exceeded.
*
* INPUTS:
*       port - the logical port number.
*       mode - the priority 1 frame rate limit mode
*              GT_FALSE: use the same rate as Pri0Rate
*              GT_TRUE:  use twice the rate as Pri0Rate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
*******************************************************************************/
GT_STATUS grcSetPri1Rate
(
    IN GT_LPORT port,
    IN GT_BOOL  mode
);

/*******************************************************************************
* grcGetPri1Rate
*
* DESCRIPTION:
*       This routine gets the ingress data rate limit for priority 1 frames.
*       Priority 1 frames will be discarded after the ingress rate selection
*       is reached or exceeded.
*
* INPUTS:
*       port - the logical port number.
*       
* OUTPUTS:
*       mode - the priority 1 frame rate limit mode
*              GT_FALSE: use the same rate as Pri0Rate
*              GT_TRUE:  use twice the rate as Pri0Rate
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
*******************************************************************************/
GT_STATUS grcGetPri1Rate
(
    IN  GT_LPORT port,
    OUT GT_BOOL  *mode
);

/*******************************************************************************
* grcSetPri0Rate
*
* DESCRIPTION:
*       This routine sets the port's ingress data limit for priority 0 frames.
*
* INPUTS:
*       port    - logical port number.
*       rate    - ingress data rate limit for priority 0 frames. These frames
*             will be discarded after the ingress rate selected is reached 
*             or exceeded. 
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
*******************************************************************************/
GT_STATUS grcSetPri0Rate
(
    IN GT_LPORT        port,
    IN GT_U32    rate
);

/*******************************************************************************
* grcGetPri0Rate
*
* DESCRIPTION:
*       This routine gets the port's ingress data limit for priority 0 frames.
*
* INPUTS:
*       port    - logical port number to set.
*
* OUTPUTS:
*       rate    - ingress data rate limit for priority 0 frames. These frames
*             will be discarded after the ingress rate selected is reached 
*             or exceeded. 
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
*******************************************************************************/
GT_STATUS grcGetPri0Rate
(
    IN  GT_LPORT port,
    OUT GT_PRI0_RATE    *rate
);

/*******************************************************************************
* grcSetBytesCount
*
* DESCRIPTION:
*       This routine sets the byets to count for limiting needs to be determined
*
* INPUTS:
*       port      - logical port number to set.
*        limitMGMT - GT_TRUE: To limit and count MGMT frame bytes
*                GT_FALSE: otherwise
*        countIFG  - GT_TRUE: To count IFG bytes
*                GT_FALSE: otherwise
*        countPre  - GT_TRUE: To count Preamble bytes
*                GT_FALSE: otherwise
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
*******************************************************************************/
GT_STATUS grcSetBytesCount
(
    IN GT_LPORT            port,
    IN GT_BOOL         limitMGMT,
    IN GT_BOOL         countIFG,
    IN GT_BOOL         countPre
);

/*******************************************************************************
* grcGetBytesCount
*
* DESCRIPTION:
*       This routine gets the byets to count for limiting needs to be determined
*
* INPUTS:
*       port    - logical port number 
*
* OUTPUTS:
*        limitMGMT - GT_TRUE: To limit and count MGMT frame bytes
*                GT_FALSE: otherwise
*        countIFG  - GT_TRUE: To count IFG bytes
*                GT_FALSE: otherwise
*        countPre  - GT_TRUE: To count Preamble bytes
*                GT_FALSE: otherwise
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
*******************************************************************************/
GT_STATUS grcGetBytesCount
(
    IN GT_LPORT            port,
    IN GT_BOOL         *limitMGMT,
    IN GT_BOOL         *countIFG,
    IN GT_BOOL         *countPre
);

/*******************************************************************************
* grcSetEgressRate
*
* DESCRIPTION:
*       This routine sets the port's egress data limit.
*        
*
* INPUTS:
*       port      - logical port number.
*       rateType  - egress data rate limit (GT_ERATE_TYPE union type). 
*                    union type is used to support multiple devices with the
*                    different formats of egress rate.
*                    GT_ERATE_TYPE has the following fields:
*                        definedRate - GT_EGRESS_RATE enum type should used for the 
*                            following devices:
*                            88E6218, 88E6318, 88E6063, 88E6083, 88E6181, 88E6183,
*                            88E6093, 88E6095, 88E6185, 88E6108, 88E6065, 88E6061, 
*                            and their variations
*                        kbRate - rate in kbps that should used for the following 
*                            devices:
*                            88E6097, 88E6096 with the GT_PIRL_ELIMIT_MODE of 
*                                GT_PIRL_ELIMIT_LAYER1,
*                                GT_PIRL_ELIMIT_LAYER2, or 
*                                GT_PIRL_ELIMIT_LAYER3 (see grcSetELimitMode)
*                            64kbps ~ 1Mbps    : increments of 64kbps,
*                            1Mbps ~ 100Mbps   : increments of 1Mbps, and
*                            100Mbps ~ 1000Mbps: increments of 10Mbps
*                            Therefore, the valid values are:
*                                64, 128, 192, 256, 320, 384,..., 960,
*                                1000, 2000, 3000, 4000, ..., 100000,
*                                110000, 120000, 130000, ..., 1000000.
*                        fRate - frame per second that should used for the following
*                            devices:
*                            88E6097, 88E6096 with GT_PIRL_ELIMIT_MODE of 
*                                GT_PIRL_ELIMIT_FRAME
*                            Valid values are between 7600 and 1488000
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
* COMMENTS: 
*            GT_16M, GT_32M, GT_64M, GT_128M, and GT_256M in GT_EGRESS_RATE enum
*            are supported only by Gigabit Ethernet Switch.
*
*******************************************************************************/
GT_STATUS grcSetEgressRate
(
    IN GT_LPORT        port,
    IN GT_ERATE_TYPE   *rateType
);

/*******************************************************************************
* grcGetEgressRate
*
* DESCRIPTION:
*       This routine gets the port's egress data limit.
*
* INPUTS:
*       port    - logical port number.
*
* OUTPUTS:
*       rateType  - egress data rate limit (GT_ERATE_TYPE union type). 
*                    union type is used to support multiple devices with the
*                    different formats of egress rate.
*                    GT_ERATE_TYPE has the following fields:
*                        definedRate - GT_EGRESS_RATE enum type should used for the 
*                            following devices:
*                            88E6218, 88E6318, 88E6063, 88E6083, 88E6181, 88E6183,
*                            88E6093, 88E6095, 88E6185, 88E6108, 88E6065, 88E6061, 
*                            and their variations
*                        kbRate - rate in kbps that should used for the following 
*                            devices:
*                            88E6097, 88E6096 with the GT_PIRL_ELIMIT_MODE of 
*                                GT_PIRL_ELIMIT_LAYER1,
*                                GT_PIRL_ELIMIT_LAYER2, or 
*                                GT_PIRL_ELIMIT_LAYER3 (see grcSetELimitMode)
*                            64kbps ~ 1Mbps    : increments of 64kbps,
*                            1Mbps ~ 100Mbps   : increments of 1Mbps, and
*                            100Mbps ~ 1000Mbps: increments of 10Mbps
*                            Therefore, the valid values are:
*                                64, 128, 192, 256, 320, 384,..., 960,
*                                1000, 2000, 3000, 4000, ..., 100000,
*                                110000, 120000, 130000, ..., 1000000.
*                        fRate - frame per second that should used for the following
*                            devices:
*                            88E6097, 88E6096 with GT_PIRL_ELIMIT_MODE of 
*                                GT_PIRL_ELIMIT_FRAME
*                            Valid values are between 7600 and 1488000
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*
* COMMENTS:
*            GT_16M, GT_32M, GT_64M, GT_128M, and GT_256M in GT_EGRESS_RATE enum
*            are supported only by Gigabit Ethernet Switch.
*
*******************************************************************************/
GT_STATUS grcGetEgressRate
(
    IN  GT_LPORT port,
    OUT GT_ERATE_TYPE  *rateType
);
GT_STATUS grcGetELimitMode
(
    IN  GT_LPORT    port,
    OUT GT_PIRL_ELIMIT_MODE        *mode
);


GT_STATUS PSW1G_RateLimitConfig(void);
void SWU_RateLimitConfig(void);

#endif//end gtPortRateCtrl
