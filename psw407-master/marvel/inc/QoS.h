#ifndef QOS_H_
#define QOS_H_


GT_STATUS miiSmiIfReadRegister(GT_U8 phyAddr,GT_U8 regAddr, GT_U16  *data);
GT_STATUS gcosSetDscp2Tc(GT_U8      dscp, IN GT_U8      trClass);
GT_STATUS hwSetGlobalRegField(GT_U8 regAddr,GT_U8 fieldOffset,GT_U8 fieldLength,GT_U16 data);
GT_STATUS gcosSetPortDefaultTc( IN GT_LPORT   port, IN GT_U8      trafClass);
GT_STATUS gcosSetDscp2Tc(GT_U8      dscp,IN GT_U8      trClass);
GT_STATUS gcosSetUserPrio2Tc(GT_U8      userPrior,GT_U8      trClass);
GT_STATUS gqosIpPrioMapEn(GT_LPORT   port,    IN GT_BOOL    en);
GT_STATUS gqosUserPrioMapEn(GT_LPORT   port,GT_BOOL    en);
GT_STATUS gqosSetPrioMapRule(GT_LPORT   port,GT_BOOL    mode);
//GT_STATUS sampleQos(void);
uint8_t   qos_set(void);


uint8_t   SWU_qos_set(void);





#endif
