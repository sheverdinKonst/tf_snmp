#ifndef _GMTRAFFICAPI_H
#define _GMTRAFFICAPI_H

typedef void (*gm_receive_packet_callback) (unsigned int uSourceDevice, unsigned int uSourcePort, unsigned char* cPacket, unsigned int size);

#ifdef __cplusplus
#ifdef BUILD_WITH_LIB
extern "C"
{
#endif
#endif

unsigned int InitTraffic(void);

unsigned int registerTrafficCallBack(gm_receive_packet_callback pCallBackMethod);

unsigned int SendPkt(unsigned int uSourceDevice, unsigned int uSourcePort, unsigned char* cPacket, unsigned int uiSize);

unsigned int ReleaseTraffic(void);

#ifdef __cplusplus
#ifdef BUILD_WITH_LIB
}
#endif
#endif

#endif /*_GMTRAFFICAPI_H*/
