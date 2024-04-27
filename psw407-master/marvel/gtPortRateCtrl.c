/********************************************************************************
* gtPortRateCtrl.c
*
* DESCRIPTION:
*       API definitions to handle port rate control registers (0xA).
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*******************************************************************************/
#include "stm32f4x7_eth.h"
#include "SMIApi.h"
#include "FlowCtrl.h"
#include "h/driver/gtDrvSwRegs.h"
#include "gtPortRateCtrl.h"
#include "QoS.h"
#include "VLAN.h"
#include "FlowCtrl.h"

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "settings.h"
#include "debug.h"
/*
 Convert given hw Rate Limit to sw defined Rate Limit.
 This routine is only for Gigabit Managed Switch Device.
 If the given device is not an accepted device, it'll simply copy the hw limit 
 to sw limit.
*/

/* The following macro converts a boolean   */
/* value to a binary one (of 1 bit).        */
/* GT_FALSE --> 0                           */
/* GT_TRUE --> 1                            */
#define BOOL_2_BIT(boolVal,binVal)                                  \
            (binVal) = (((boolVal) == GT_TRUE) ? 1 : 0)
#define GT_IS_PORT_SET(_portVec, _port)    \
            ((_portVec) & (0x1 << (_port)))

#define RECOMMENDED_ESB_LIMIT(_bps) 0xFFFFFF

#define RECOMMENDED_CBS_LIMIT(_bps) 0x200000

#define RECOMMENDED_BUCKET_INCREMENT(_bps)               \
        ((0/*IS_IN_DEV_GROUP(dev,DEV_PIRL_RESOURCE)*/)?174:   \
        ((_bps) < 1000)?0x3d:                            \
        ((_bps) < 10000)?0x1f:0x4)

/* The following macro converts a binary    */
/* value (of 1 bit) to a boolean one.       */
/* 0 --> GT_FALSE                           */
/* 1 --> GT_TRUE                            */
#define BIT_2_BOOL(binVal,boolVal)                                  \
            (boolVal) = (((binVal) == 0) ? GT_FALSE : GT_TRUE)

static GT_STATUS pirl2OperationPerform(GT_PIRL2_OPERATION pirlOp,GT_PIRL2_OP_DATA *opData);
static GT_STATUS pirl2InitIRLResource(GT_U32  irlPort,GT_U32 irlRes);
static GT_STATUS pirl2DataToResource(GT_PIRL2_DATA *pirlData,GT_PIRL2_RESOURCE    *res);
static GT_STATUS gpirl2SetCurTimeUpInt(GT_U32  upInt);
static GT_STATUS pirl2WriteResource(GT_U32  irlPort,GT_U32 irlRes,GT_PIRL2_RESOURCE *res);
static GT_STATUS gpirl2WriteResource(GT_LPORT port, GT_U32 irlRes,  GT_PIRL2_DATA *pirlData);
static GT_STATUS setEnhancedERate(GT_LPORT port, GT_ERATE_TYPE *rateType);

#if 0
static GT_STATUS cRateLimit(GT_U32 hwLimit, GT_U32* swLimit)
{
    GT_U32 sLimit, hLimit, startLimit, endLimit, i;

    /*if (!((IS_IN_DEV_GROUP(dev,DEV_GIGABIT_MANAGED_SWITCH)) ||
        (IS_IN_DEV_GROUP(dev,DEV_ENHANCED_FE_SWITCH)) ||
		(IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))))
    {
        *swLimit = hwLimit;
        return GT_OK;
    }*/

    if(hwLimit == 0)
    {
        *swLimit = GT_NO_LIMIT;
        return GT_OK;
    }
        
    sLimit = 1000;

    if (/*(IS_IN_DEV_GROUP(dev,DEV_ENHANCED_FE_SWITCH)*/1) //||
		//(IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY)))

			hLimit = GT_GET_RATE_LIMIT(sLimit);

    if(hLimit == hwLimit)
    {
        *swLimit = GT_1M;
        return GT_OK;
    }
    
    if(hLimit > hwLimit)
    {
        startLimit = 2000;
        endLimit = 256000;
        *swLimit = GT_2M;
    }
    else
    {
        startLimit = 128;
        endLimit = 512;
        *swLimit = GT_128K;
    }
    
    i = 0;
    for(sLimit=startLimit;sLimit<=endLimit;sLimit *= 2, i++)
    {
          hLimit = GT_GET_RATE_LIMIT(sLimit);

        if(hLimit == 0)
            hLimit = 1;

        if(hLimit == hwLimit)
        {
            *swLimit += i;
            return GT_OK;
        }

        if(hLimit < hwLimit)
            break;
    }

    *swLimit = hwLimit;
    return GT_OK;
}


/*
 Convert given sw defined Burst Rate to meaningful number.
*/
static GT_STATUS cBurstEnum2Number(GT_BURST_RATE rate, GT_U32 *rLimit)
{
    GT_U32 rateLimit;


    switch(rate)
    {
        case GT_BURST_NO_LIMIT :
                rateLimit = 0; /* MAX_RATE_LIMIT; */
                break;
        case GT_BURST_64K :
                rateLimit = 64;
                break;
        case GT_BURST_128K :
                rateLimit = 128;
                break;
        case GT_BURST_256K :
                rateLimit = 256;
                break;
        case GT_BURST_384K :
                rateLimit = 384;
                break;
        case GT_BURST_512K :
                rateLimit = 512;
                break;
        case GT_BURST_640K :
                rateLimit = 640;
                break;
        case GT_BURST_768K :
                rateLimit = 768;
                break;
        case GT_BURST_896K :
                rateLimit = 896;
                break;
        case GT_BURST_1M :
                rateLimit = 1000;
                break;
        case GT_BURST_1500K :
                rateLimit = 1500;
                break;
        case GT_BURST_2M :
                rateLimit = 2000;
                break;
        case GT_BURST_4M :
                rateLimit = 4000;
                break;
        case GT_BURST_8M :
                rateLimit = 8000;
                break;
        case GT_BURST_16M :
                rateLimit = 16000;
                break;
        case GT_BURST_32M :
                rateLimit = 32000;
                break;
        case GT_BURST_64M :
                rateLimit = 64000;
                break;
        case GT_BURST_128M :
                rateLimit = 128000;
                break;
        case GT_BURST_256M :
                rateLimit = 256000;
                break;
        default :
                return GT_BAD_PARAM;
    }

    *rLimit = rateLimit;
    return GT_OK;
}


/*
 Convert given hw Burst Rate Limit to sw defined Burst Rate Limit.
*/
static GT_STATUS cBurstRateLimit(GT_U32 burstSize, GT_U32 hwLimit, GT_BURST_RATE* swLimit)
{
    GT_BURST_RATE sLimit, startLimit, endLimit;
    GT_U32 rLimit, tmpLimit;
    GT_STATUS       retVal;         /* Functions return value.      */

    if(hwLimit == 0)
    {
        *swLimit = GT_BURST_NO_LIMIT;
        return GT_OK;
    }
        
    startLimit = GT_BURST_64K;
    endLimit = GT_BURST_256M;
    
    for(sLimit=startLimit;sLimit<=endLimit;sLimit++)
    {
        if((retVal = cBurstEnum2Number(sLimit, &rLimit)) != GT_OK)
        {
            DBG_INFO(("Failed.\r\n"));
               return retVal;
        }

        tmpLimit = GT_GET_BURST_RATE_LIMIT(burstSize,rLimit);

        if(hwLimit == tmpLimit)
        {
            *swLimit = sLimit;
            return GT_OK;
        }
    }

    return GT_FAIL;
}


/*
 Convert given sw defined Burst Rate to meaningful number.
*/
static GT_STATUS cTCPBurstRate(GT_BURST_RATE rate, GT_U32 *data)
{

    switch(rate)
    {
        case GT_BURST_NO_LIMIT :
                *data = 0; /* MAX_RATE_LIMIT; */
                break;
        case GT_BURST_64K :
                *data = 0x1D00;
                break;
        case GT_BURST_128K :
                *data = 0x3FFF;
                break;
        case GT_BURST_256K :
                *data = 0x7FFF;
                break;
        case GT_BURST_384K :
                *data = 0x7DE0;
                break;
        case GT_BURST_512K :
                *data = 0x76F0;
                break;
        case GT_BURST_640K :
                *data = 0x7660;
                break;
        case GT_BURST_768K :
                *data = 0x7600;
                break;
        case GT_BURST_896K :
                *data = 0x74EF;
                break;
        case GT_BURST_1M :
                *data = 0x7340;
                break;
        case GT_BURST_1500K :
                *data = 0x7300;
                break;
        default :
                return GT_BAD_PARAM;
    }

    return GT_OK;
}

GT_STATUS setEnhancedERate(GT_LPORT port, GT_ERATE_TYPE *rateType)
{
    GT_STATUS    retVal;         /* Functions return value.      */
    GT_U16        data;
    GT_U32        rate, eDec;
    GT_PIRL_ELIMIT_MODE        mode;
    GT_U8        phyPort;        /* Physical port.               */

    phyPort = lport2port(port);

    if((retVal = grcGetELimitMode(port,&mode)) != GT_OK)
    {
        return retVal;
    }

    if (mode == GT_PIRL_ELIMIT_FRAME)    
    {
        /* Count Per Frame */
        rate = rateType->fRate;

        if (rate == 0) /* disable egress rate limit */
        {
            eDec = 0;
            data = 0;
        }
        else if((rate < 7600)  || (rate > 1488000))
        {
            return GT_BAD_PARAM;
        }
        else
        {
            eDec = 1;
            data = (GT_U16)GT_GET_RATE_LIMIT_PER_FRAME(rate,eDec);
        }
    }
    else
    {
        /* Count Per Byte */
        rate = rateType->kbRate;

        if(rate == 0)
        {
            eDec = 0;
        }
        else if(rate < 1000)    /* less than 1Mbps */
        {
            /* it should be divided by 64 */
            if(rate % 64)
                return GT_BAD_PARAM;
            eDec = rate/64;
        }
        else if(rate <= 100000)    /* less than or equal to 100Mbps */
        {
            /* it should be divided by 1000 */
            if(rate % 1000)
                return GT_BAD_PARAM;
            eDec = rate/1000;
        }
        else if(rate <= 1000000)    /* less than or equal to 1000Mbps */
        {
            /* it should be divided by 10000 */
            if(rate % 10000)
                return GT_BAD_PARAM;
            eDec = rate/10000;
        }
        else
            return GT_BAD_PARAM;

        if(rate == 0)
        {
            data = 0;
        }
        else
        {
            data = (GT_U16)GT_GET_RATE_LIMIT_PER_BYTE(rate,eDec);
        }
    }

    retVal = hwSetPortRegField(phyPort,QD_REG_RATE_CTRL0,0,7,(GT_U16)eDec);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\r\n"));
        return retVal;
    }

    retVal = hwSetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,0,12,(GT_U16)data );
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\n"));
           return retVal;
    }

    DBG_INFO(("OK.\r\n"));
    return GT_OK;
}


GT_STATUS getEnhancedERate(GT_LPORT port, GT_ERATE_TYPE *rateType)
{
    GT_STATUS    retVal;         /* Functions return value.      */
    GT_U16        rate, eDec;
    GT_PIRL_ELIMIT_MODE        mode;
    GT_U8        phyPort;        /* Physical port.               */

    phyPort = lport2port(port);

    if((retVal = grcGetELimitMode(port,&mode)) != GT_OK)
    {
        return retVal;
    }

    retVal = hwGetPortRegField(phyPort,QD_REG_RATE_CTRL0,0,7,&eDec);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\r\n"));
        return retVal;
    }

    retVal = hwGetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,0,12,&rate );
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\n"));
           return retVal;
    }

    if (mode == GT_PIRL_ELIMIT_FRAME)    
    {
        rateType->fRate = GT_GET_RATE_LIMIT_PER_FRAME(rate,eDec);
    }
    else
    {
        /* Count Per Byte */
        rateType->kbRate = GT_GET_RATE_LIMIT_PER_BYTE(rate,eDec);
    }

    DBG_INFO(("OK.\r\n"));
    return GT_OK;
}

#endif
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
*
* COMMENTS: 
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcSetLimitMode
(
    IN GT_LPORT          port,
    IN GT_RATE_LIMIT_MODE    mode
)
{

    //GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           phyPort;        /* Physical port.               */

    DEBUG_MSG(DEBUG_QD,"grcSetLimitMode Called.\r\n");

    phyPort = lport2port(port);

   	hwSetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,14,2,(GT_U16)mode);

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}


#if 0
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
* GalTis:
*
*******************************************************************************/
GT_STATUS grcGetLimitMode
(
    IN  GT_LPORT port,
    OUT GT_RATE_LIMIT_MODE    *mode
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* The register's read data.    */
    GT_U8           phyPort;        /* Physical port.               */

    DBG_INFO(("grcGetLimitMode Called.\r\n"));

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_INGRESS_RATE_KBPS)) != GT_OK )
    //  return retVal;
    
    if(mode == NULL)
    {
        DBG_INFO(("Failed.\r\n"));
        return GT_BAD_PARAM;
    }

    if (/*IS_IN_DEV_GROUP(dev,DEV_GIGABIT_MANAGED_SWITCH)*/1)
    {
        retVal = hwGetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,14,2,&data );
    }
    else
    {
        retVal = hwGetPortRegField(phyPort,QD_REG_RATE_CTRL,14,2,&data );
    }
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\r\n"));
        return retVal;
    }

    *mode = data;
    DBG_INFO(("OK.\r\n"));
    return GT_OK;
}
#endif
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
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcSetPri3Rate
(
    IN GT_LPORT port,
    IN GT_BOOL  mode
)
{
    //GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* Data to be set into the      */
                                    /* register.                    */
    GT_U8           phyPort;        /* Physical port.               */

    DEBUG_MSG(DEBUG_QD,"grcSetPri3Rate Called.\r\n");

    phyPort = lport2port(port);
    
    BOOL_2_BIT(mode,data);

    hwSetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,14,1,data );

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}


#if 0
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
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcGetPri3Rate
(
    IN  GT_LPORT port,
    OUT GT_BOOL  *mode
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* The register's read data.    */
    GT_U8           phyPort;        /* Physical port.               */

    DBG_INFO(("grcGetPri3Rate Called.\r\n"));
    if(mode == NULL)
    {
        DBG_INFO(("Failed.\r\n"));
        return GT_BAD_PARAM;
    }

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_INGRESS_RATE_KBPS)) != GT_OK )
    //  return retVal;
    
    retVal = hwGetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,14,1,&data );

    BIT_2_BOOL(data,*mode);
    DBG_INFO(("OK.\r\n"));
    return GT_OK;
}

#endif
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
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcSetPri2Rate
(
    IN GT_LPORT port,
    IN GT_BOOL  mode
)
{
    //GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* Data to be set into the      */
                                    /* register.                    */
    GT_U8           phyPort;        /* Physical port.               */

    DEBUG_MSG(DEBUG_QD,"grcSetPri2Rate Called.\r\n");

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_INGRESS_RATE_KBPS)) != GT_OK )
    //  return retVal;
    BOOL_2_BIT(mode,data);
    hwSetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,13,1,data );
    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}


#if 0
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
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcGetPri2Rate
(
    IN  GT_LPORT port,
    OUT GT_BOOL  *mode
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* The register's read data.    */
    GT_U8           phyPort;        /* Physical port.               */

    DBG_INFO(("grcGetPri2Rate Called.\r\n"));
    if(mode == NULL)
    {
        DBG_INFO(("Failed.\r\n"));
        return GT_BAD_PARAM;
    }

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_INGRESS_RATE_KBPS)) != GT_OK )
    //  return retVal;
    
    retVal = hwGetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,13,1,&data );

    BIT_2_BOOL(data,*mode);
    DBG_INFO(("OK.\r\n"));
    return GT_OK;
}
#endif

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
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcSetPri1Rate
(
    IN GT_LPORT  port,
    IN GT_BOOL   mode
)
{
    //GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* Data to be set into the      */
                                    /* register.                    */
    GT_U8           phyPort;        /* Physical port.               */

    DEBUG_MSG(DEBUG_QD,"grcSetPri1Rate Called.\r\n");

    phyPort = lport2port(port);
    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_INGRESS_RATE_KBPS)) != GT_OK )
    //  return retVal;
    
    BOOL_2_BIT(mode,data);

    hwSetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,12,1,data );

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}


#if 0
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
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcGetPri1Rate
(
    IN  GT_LPORT port,
    OUT GT_BOOL  *mode
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* The register's read data.    */
    GT_U8           phyPort;        /* Physical port.               */

    DBG_INFO(("grcGetPri1Rate Called.\r\n"));
    if(mode == NULL)
    {
        DBG_INFO(("Failed.\r\n"));
        return GT_BAD_PARAM;
    }

    phyPort = lport2port(port);

    retVal = hwGetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,12,1,&data );

    BIT_2_BOOL(data,*mode);
    DBG_INFO(("OK.\r\n"));
    return GT_OK;
}

#endif
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
* COMMENTS: 
*            GT_16M, GT_32M, GT_64M, GT_128M, and GT_256M in GT_PRI0_RATE enum
*            are supported only by Gigabit Ethernet Switch.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcSetPri0Rate
(
    IN GT_LPORT        port,
    IN GT_U32    rate
)
{

   // GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U32            rateLimit=0, tmpLimit;

    DEBUG_MSG(DEBUG_QD,"grcSetPri0Rate Called.\r\n");

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(phyPort, DEV_INGRESS_RATE_KBPS|DEV_UNMANAGED_SWITCH)) != GT_OK )
    //  return retVal;

        //dev->devStorage &= ~(GT_RATE_ENUM_NOT_USED);
 /*       switch(rate)
        {
            case GT_NO_LIMIT :
                    rateLimit = 0; // MAX_RATE_LIMIT; //
                    break;
            case GT_128K :
                    rateLimit = 128;
                    break;
            case GT_256K :
                    rateLimit = 256;
                    break;
            case GT_512K :
                    rateLimit = 512;
                    break;
            case GT_1M :
                    rateLimit = 1000;
                    break;
            case GT_2M :
                    rateLimit = 2000;
                    break;
            case GT_4M :
                    rateLimit = 4000;
                    break;
            case GT_8M :
                    rateLimit = 8000;
                    break;
            case GT_16M :
                    rateLimit = 16000;
                    break;
            case GT_32M :
                    rateLimit = 32000;
                    break;
            case GT_64M :
                    rateLimit = 64000;
                    break;
            case GT_128M :
                    rateLimit = 128000;
                    break;
            case GT_256M :
                    rateLimit = 256000;
                    break;
            default :
                    rateLimit = (GT_U32)rate;
                    //dev->devStorage |= GT_RATE_ENUM_NOT_USED;
                    break;                    
        }
*/

		rateLimit = (GT_U32)rate;
        tmpLimit = GT_GET_RATE_LIMIT2(rateLimit);

        if((tmpLimit == 0) && (rateLimit != 0))
            rateLimit = 1;
        else
            rateLimit = tmpLimit;

        hwSetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,0,12,(GT_U16)rateLimit );

    DEBUG_MSG(DEBUG_QD,"OK.\r\r\n");
    return GT_OK;
}


#if 0
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
* COMMENTS: 
*            GT_16M, GT_32M, GT_64M, GT_128M, and GT_256M in GT_PRI0_RATE enum
*            are supported only by Gigabit Ethernet Switch.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcGetPri0Rate
(
    IN  GT_LPORT port,
    OUT GT_PRI0_RATE    *rate
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* The register's read data.    */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U32            tmpLimit;

    DBG_INFO(("grcGetPri0Rate Called.\r\r\n"));

    if(rate == NULL)
    {
        DBG_INFO(("Failed.\r\r\n"));
        return GT_BAD_PARAM;
    }

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_INGRESS_RATE_KBPS|DEV_UNMANAGED_SWITCH)) != GT_OK )
    //  return retVal;

    //if (IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH))
    //{
        retVal = hwGetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,0,12,&data);
        tmpLimit = (GT_U32)data;

        if(retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }


            cRateLimit(tmpLimit, (GT_U32*)rate);

   /* }
    else
    {
        retVal = hwGetPortRegField(dev,phyPort,QD_REG_RATE_CTRL,8,3,&data );
        if(retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }
        *rate = data;
    }*/

    DBG_INFO(("OK.\r\r\n"));
    return GT_OK;
}

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
* COMMENTS: 
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcSetBytesCount
(
    IN GT_LPORT  port,
    IN GT_BOOL      limitMGMT,
    IN GT_BOOL      countIFG,
    IN GT_BOOL      countPre
)
{

    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U16          data;           /* data for bytes count         */

    DBG_INFO(("grcSetBytesCount Called.\r\r\n"));

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_INGRESS_RATE_KBPS|DEV_UNMANAGED_SWITCH)) != GT_OK )
    //  return retVal;

    if (/*IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH)*/1)
    {
        BOOL_2_BIT(limitMGMT,data);
        retVal = hwSetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,15,1,data );
        if (retVal != GT_OK)
            return retVal;

        data = 0;
        if( countIFG == GT_TRUE ) data |= 2;
        if( countPre == GT_TRUE ) data |= 1;

        retVal = hwSetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,12,2,data );
    }
    else
    {
        data = 0;
        if(    limitMGMT == GT_TRUE ) data |=4;
        if(     countIFG == GT_TRUE ) data |=2;
        if(     countPre == GT_TRUE ) data |=1;

        retVal = hwSetPortRegField(phyPort,QD_REG_RATE_CTRL,4,3,data );
    }

       if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\r\r\n"));
        return retVal;
    }
    DBG_INFO(("OK.\r\r\n"));
    return GT_OK;
}



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
* GalTis:
*
*******************************************************************************/
GT_STATUS grcGetBytesCount
(    
    IN GT_LPORT  port,
    IN GT_BOOL      *limitMGMT,
    IN GT_BOOL      *countIFG,
    IN GT_BOOL      *countPre
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* The register's read data.    */
    GT_U8           phyPort;        /* Physical port.               */

    DBG_INFO(("grcGetBytesCount Called.\r\r\n"));

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_INGRESS_RATE_KBPS|DEV_UNMANAGED_SWITCH)) != GT_OK )
    //  return retVal;

    if (limitMGMT == NULL || countIFG == NULL || countPre == NULL)
    {
        DBG_INFO(("Failed.\r\r\n"));
        return GT_BAD_PARAM;
    }
       *limitMGMT = *countIFG = *countPre = GT_FALSE;

    if (/*IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH)*/1)
    {
        retVal = hwGetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,15,1,&data );
        if (retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }

        BIT_2_BOOL(data,*limitMGMT);
        retVal = hwGetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,12,2,&data );
        if (retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }

        if( data & 0x2 ) *countIFG = GT_TRUE;
        if( data & 0x1 ) *countPre = GT_TRUE;

    }
    else
    {

        retVal = hwGetPortRegField(phyPort,QD_REG_RATE_CTRL,4,3,&data );
        if(retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }

        if ( data & 4 ) *limitMGMT = GT_TRUE;
        if ( data & 2 ) *countIFG  = GT_TRUE;
        if ( data & 1 ) *countPre  = GT_TRUE;
    
    }
        
    DBG_INFO(("OK.\r\r\n"));
    return GT_OK;
}

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
)
{

    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U32            rateLimit, tmpLimit;
    GT_EGRESS_RATE  rate;

    DBG_INFO(("grcSetEgressRate Called.\r\r\n"));

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_EGRESS_RATE_KBPS|DEV_UNMANAGED_SWITCH)) != GT_OK )
    //  return retVal;
    
    //if (IS_IN_DEV_GROUP(dev,DEV_ELIMIT_FRAME_BASED))
    //{
    //    return setEnhancedERate(dev,port,rateType);
    //}

    rate = rateType->definedRate;

    if (/*(IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH)) ||
        (IS_IN_DEV_GROUP(dev,DEV_ENHANCED_FE_SWITCH)) ||
		(IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))*/1)
    {
        //dev->devStorage &= ~(GT_RATE_ENUM_NOT_USED);
        switch(rate)
        {
            case GT_NO_LIMIT :
                    rateLimit = 0; /* MAX_RATE_LIMIT; */
                    break;
            case GT_128K :
                    rateLimit = 128;
                    break;
            case GT_256K :
                    rateLimit = 256;
                    break;
            case GT_512K :
                    rateLimit = 512;
                    break;
            case GT_1M :
                    rateLimit = 1000;
                    break;
            case GT_2M :
                    rateLimit = 2000;
                    break;
            case GT_4M :
                    rateLimit = 4000;
                    break;
            case GT_8M :
                    rateLimit = 8000;
                    break;
            case GT_16M :
                    rateLimit = 16000;
                    break;
            case GT_32M :
                    rateLimit = 32000;
                    break;
            case GT_64M :
                    rateLimit = 64000;
                    break;
            case GT_128M :
                    rateLimit = 128000;
                    break;
            case GT_256M :
                    rateLimit = 256000;
                    break;
            default :
                    rateLimit = (GT_U32)rate;
                    //dev->devStorage |= GT_RATE_ENUM_NOT_USED;
                    break;                    
        }


            tmpLimit = GT_GET_RATE_LIMIT(rateLimit);

        if((tmpLimit == 0) && (rateLimit != 0))
            rateLimit = 1;
        else
            rateLimit = tmpLimit;

        retVal = hwSetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,0,12,(GT_U16)rateLimit );
        if(retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }
    }
    else
    {
        switch(rate)
        {
            case GT_NO_LIMIT :
            case GT_128K :
            case GT_256K :
            case GT_512K :
            case GT_1M :
            case GT_2M :
            case GT_4M :
            case GT_8M :
                    break;
            default :
                    return GT_BAD_PARAM;
        }
        retVal = hwSetPortRegField(phyPort,QD_REG_RATE_CTRL,0,3,(GT_U16)rate );
        if(retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }
    }

    DBG_INFO(("OK.\r\r\n"));
    return GT_OK;
}



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
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* The register's read data.    */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U32            tmpLimit,tmpRate;

    DBG_INFO(("grcGetEgressRate Called.\r\r\n"));

    phyPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,phyPort, DEV_EGRESS_RATE_KBPS|DEV_UNMANAGED_SWITCH)) != GT_OK )
    //  return retVal;
    
    if(rateType == NULL)
    {
        DBG_INFO(("Failed.\r\r\n"));
        return GT_BAD_PARAM;
    }

    //if (IS_IN_DEV_GROUP(dev,DEV_ELIMIT_FRAME_BASED))
    //{
    //    return getEnhancedERate(dev,port,rateType);
    //}

    if (/*(IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH)) ||
        (IS_IN_DEV_GROUP(dev,DEV_ENHANCED_FE_SWITCH)) ||
		(IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))*/1)
    {
        retVal = hwGetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,0,12,&data );
        tmpLimit = (GT_U32)data;
        if(retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }

        /*if(dev->devStorage & GT_RATE_ENUM_NOT_USED)
        {
            if ((IS_IN_DEV_GROUP(dev,DEV_ENHANCED_FE_SWITCH)) ||
	        	(IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY)))
                tmpRate = GT_GET_RATE_LIMIT3(tmpLimit);
            else if (!IS_IN_DEV_GROUP(dev,DEV_88E6183_FAMILY))
                tmpRate = GT_GET_RATE_LIMIT2(tmpLimit);
            else
                tmpRate = GT_GET_RATE_LIMIT(tmpLimit);
            rateType->kbRate = tmpRate;
        }
        else
        {*/
            cRateLimit(tmpLimit, &tmpRate);
            rateType->definedRate = (GT_EGRESS_RATE)tmpRate;
        //}
    }
    else
    {
        retVal = hwGetPortRegField(phyPort,QD_REG_RATE_CTRL,0,3,&data );
        if(retVal != GT_OK)
        {
            DBG_INFO(("Failed.\r\r\n"));
            return retVal;
        }
        
        rateType->definedRate = (GT_EGRESS_RATE)data;
    }


    DBG_INFO(("OK.\r\r\n"));
    return GT_OK;
}


/*******************************************************************************
* grcSetBurstRate
*
* DESCRIPTION:
*       This routine sets the port's ingress data limit based on burst size.
*
* INPUTS:
*       port    - logical port number.
*       bsize    - burst size.
*       rate    - ingress data rate limit. These frames will be discarded after 
*                the ingress rate selected is reached or exceeded. 
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters 
*                                Minimum rate for Burst Size 24K byte is 128Kbps
*                                Minimum rate for Burst Size 48K byte is 256Kbps
*                                Minimum rate for Burst Size 96K byte is 512Kbps
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*        If the device supports both priority based Rate Limiting and burst size
*        based Rate limiting, user has to manually change the mode to burst size
*        based Rate limiting by calling gsysSetRateLimitMode.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcSetBurstRate
(
    IN GT_LPORT        port,
    IN GT_BURST_SIZE   bsize,
    IN GT_BURST_RATE   rate
)
{

    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U32            rateLimit;
    GT_U32            burstSize =0;

    DBG_INFO(("grcSetBurstRate Called.\r\r\n"));

    phyPort = lport2port(port);

    /* check if the given Switch supports this feature. */
   /* if (!IS_IN_DEV_GROUP(dev,DEV_BURST_RATE))
    {
        if (!IS_IN_DEV_GROUP(dev,DEV_NEW_FEATURE_IN_REV) || 
            ((GT_DEVICE_REV)dev->revision < GT_REV_1))
        {
            DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
            return GT_NOT_SUPPORTED;
        }
    }*/

    switch (bsize)
    {
        case GT_BURST_SIZE_12K:
            burstSize = 0;
            break;
        case GT_BURST_SIZE_24K:
            if ((rate < GT_BURST_128K) && (rate != GT_BURST_NO_LIMIT))
                return GT_BAD_PARAM;
            burstSize = 1;
            break;
        case GT_BURST_SIZE_48K:
            if ((rate < GT_BURST_256K) && (rate != GT_BURST_NO_LIMIT))
                return GT_BAD_PARAM;
            burstSize = 3;
            break;
        case GT_BURST_SIZE_96K:
            if ((rate < GT_BURST_512K) && (rate != GT_BURST_NO_LIMIT))
                return GT_BAD_PARAM;
            burstSize = 7;
            break;
        default:
            return GT_BAD_PARAM;
    }

    if((retVal = cBurstEnum2Number(rate, &rateLimit)) != GT_OK)
    {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    rateLimit = GT_GET_BURST_RATE_LIMIT(burstSize,rateLimit);

    rateLimit |= (GT_U32)(burstSize << 12);

    retVal = hwSetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,0,15,(GT_U16)rateLimit );
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }
    DBG_INFO(("OK.\r\r\n"));
    return GT_OK;
}


/*******************************************************************************
* grcGetBurstRate
*
* DESCRIPTION:
*       This routine retrieves the port's ingress data limit based on burst size.
*
* INPUTS:
*       port    - logical port number.
*
* OUTPUTS:
*       bsize    - burst size.
*       rate    - ingress data rate limit. These frames will be discarded after 
*                the ingress rate selected is reached or exceeded. 
*
* RETURNS:
*       GT_OK            - on success
*       GT_FAIL          - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS: 
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcGetBurstRate
(
    IN  GT_LPORT        port,
    OUT GT_BURST_SIZE   *bsize,
    OUT GT_BURST_RATE   *rate
)
{

    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U32            rateLimit, burstSize;
    GT_U16            data;

    DBG_INFO(("grcGetBurstRate Called.\r\r\n"));

    phyPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    /*if (!IS_IN_DEV_GROUP(dev,DEV_BURST_RATE))
    {
        if (!IS_IN_DEV_GROUP(dev,DEV_NEW_FEATURE_IN_REV) || 
            ((GT_DEVICE_REV)dev->revision < GT_REV_1))
        {
            DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
            return GT_NOT_SUPPORTED;
        }
    }
    */
    retVal = hwGetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,0,15,&data);
    rateLimit = (GT_U32)data;
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    burstSize = rateLimit >> 12;
    rateLimit &= 0x0FFF;

    retVal = cBurstRateLimit(burstSize, rateLimit, rate);
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    switch (burstSize)
    {
        case 0:
            *bsize = GT_BURST_SIZE_12K;
            break;
        case 1:
            *bsize = GT_BURST_SIZE_24K;
            break;
        case 3:
            *bsize = GT_BURST_SIZE_48K;
            break;
        case 7:
            *bsize = GT_BURST_SIZE_96K;
            break;
        default:
            return GT_BAD_VALUE;
    }

    DBG_INFO(("OK.\r\r\n"));
    return GT_OK;
}


/*******************************************************************************
* grcSetTCPBurstRate
*
* DESCRIPTION:
*       This routine sets the port's TCP/IP ingress data limit based on burst size.
*
* INPUTS:
*       port    - logical port number.
*       rate    - ingress data rate limit for TCP/IP packets. These frames will 
*                be discarded after the ingress rate selected is reached or exceeded. 
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters 
*                                Valid rate is GT_BURST_NO_LIMIT, or between
*                                64Kbps and 1500Kbps.
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*        If the device supports both priority based Rate Limiting and burst size
*        based Rate limiting, user has to manually change the mode to burst size
*        based Rate limiting by calling gsysSetRateLimitMode.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcSetTCPBurstRate
(
    IN GT_LPORT        port,
    IN GT_BURST_RATE   rate
)
{

    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U32            rateLimit;

    DBG_INFO(("grcSetTCPBurstRate Called.\r\r\n"));

    phyPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    /*if (!IS_IN_DEV_GROUP(dev,DEV_BURST_RATE))
    {
        if (!IS_IN_DEV_GROUP(dev,DEV_NEW_FEATURE_IN_REV) || 
            ((GT_DEVICE_REV)dev->revision < GT_REV_1))
        {
            DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
            return GT_NOT_SUPPORTED;
        }
    }
    */

    if((retVal = cTCPBurstRate(rate, &rateLimit)) != GT_OK)
    {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    retVal = hwSetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,0,15,(GT_U16)rateLimit );
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }
    DBG_INFO(("OK.\r\r\n"));
    return GT_OK;
}



/*******************************************************************************
* grcGetTCPBurstRate
*
* DESCRIPTION:
*       This routine sets the port's TCP/IP ingress data limit based on burst size.
*
* INPUTS:
*       port    - logical port number.
*
* OUTPUTS:
*       rate    - ingress data rate limit for TCP/IP packets. These frames will 
*                be discarded after the ingress rate selected is reached or exceeded. 
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_VALUE        - register value is not known
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*        If the device supports both priority based Rate Limiting and burst size
*        based Rate limiting, user has to manually change the mode to burst size
*        based Rate limiting by calling gsysSetRateLimitMode.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS grcGetTCPBurstRate
(
    IN  GT_LPORT        port,
    OUT GT_BURST_RATE   *rate
)
{

    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           phyPort;        /* Physical port.               */
    GT_U32            rateLimit;
    GT_U32            data;
    GT_U16            u16Data;
    GT_BURST_RATE sLimit, startLimit, endLimit;

    DBG_INFO(("grcGetTCPBurstRate Called.\r\r\n"));

    phyPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    /*if (!IS_IN_DEV_GROUP(dev,DEV_BURST_RATE))
    {
        if (!IS_IN_DEV_GROUP(dev,DEV_NEW_FEATURE_IN_REV) || 
            ((GT_DEVICE_REV)dev->revision < GT_REV_1))
        {
            DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
            return GT_NOT_SUPPORTED;
        }
    }

    */

    retVal = hwGetPortRegField(phyPort,QD_REG_INGRESS_RATE_CTRL,0,15,&u16Data);
    data = (GT_U32)u16Data;
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    if ((data & 0xFFF) == 0)
    {
        *rate = GT_BURST_NO_LIMIT;
        return GT_OK;
    }

    startLimit = GT_BURST_64K;
    endLimit = GT_BURST_1500K;

    for(sLimit=startLimit;sLimit<=endLimit;sLimit++)
    {
        if((retVal = cTCPBurstRate(sLimit, &rateLimit)) != GT_OK)
        {
            break;
        }

        if(rateLimit == data)
        {
            *rate = sLimit;
            return GT_OK;
        }
    }

    DBG_INFO(("Fail to find TCP Rate.\r\r\n"));
    return GT_BAD_VALUE;
}


/*******************************************************************************
* grcSetVidNrlEn
*
* DESCRIPTION:
*       This routine enables/disables VID None Rate Limit (NRL).
*        When VID NRL is enabled and the determined VID of a frame results in a VID
*        whose VIDNonRateLimit in the VTU Table is set to GT_TURE, then the frame
*        will not be ingress nor egress rate limited.
*
* INPUTS:
*       port - logical port number.
*        mode - GT_TRUE to enable VID None Rate Limit
*               GT_FALSE otherwise
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcSetVidNrlEn
(
    IN  GT_LPORT    port,
    IN  GT_BOOL        mode
)
{
    GT_U16          data;           
    //GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;        /* Physical port.               */

    DBG_INFO(("grcSetVidNrlEn Called.\r\r\n"));

    hwPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    //if (!IS_IN_DEV_GROUP(dev,DEV_NONE_RATE_LIMIT))
    //{
           DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    //}

    /* translate BOOL to binary */
    BOOL_2_BIT(mode, data);

    /* Set the VidNrlEn mode.            */
 /*   retVal = hwSetPortRegField(dev,hwPort, QD_REG_INGRESS_RATE_CTRL,15,1,data);
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    DBG_INFO(("OK.\r\r\n"));
*/
    return GT_OK;
}

/*******************************************************************************
* grcGetVidNrlEn
*
* DESCRIPTION:
*       This routine gets VID None Rate Limit (NRL) mode.
*        When VID NRL is enabled and the determined VID of a frame results in a VID
*        whose VIDNonRateLimit in the VTU Table is set to GT_TURE, then the frame
*        will not be ingress nor egress rate limited.
*
* INPUTS:
*       port - logical port number.
*
* OUTPUTS:
*        mode - GT_TRUE to enable VID None Rate Limit
*               GT_FALSE otherwise
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcGetVidNrlEn
(
    IN  GT_LPORT    port,
    OUT GT_BOOL        *mode
)
{
    GT_U16          data;           
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;        /* Physical port.               */

    DBG_INFO(("grcGetVidNrlEn Called.\r\r\n"));

    hwPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_NONE_RATE_LIMIT)*/1)
    {
           DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }

    /* Get the VidNrlEn mode.            */
    retVal = hwGetPortRegField(hwPort, QD_REG_INGRESS_RATE_CTRL,15,1,&data);
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    BIT_2_BOOL(data, *mode);

    DBG_INFO(("OK.\r\r\n"));

    return GT_OK;
}


/*******************************************************************************
* grcSetSaNrlEn
*
* DESCRIPTION:
*       This routine enables/disables SA None Rate Limit (NRL).
*        When SA NRL is enabled and the source address of a frame results in a ATU
*        hit where the SA's MAC address returns an EntryState that indicates Non
*        Rate Limited, then the frame will not be ingress nor egress rate limited.
*
* INPUTS:
*       port - logical port number.
*        mode - GT_TRUE to enable SA None Rate Limit
*               GT_FALSE otherwise
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcSetSaNrlEn
(
    IN  GT_LPORT    port,
    IN  GT_BOOL        mode
)
{
    GT_U16          data;           
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;        /* Physical port.               */

    DBG_INFO(("grcSetSaNrlEn Called.\r\r\n"));

    hwPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_NONE_RATE_LIMIT)*/1)
    {
           DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }

    /* translate BOOL to binary */
    BOOL_2_BIT(mode, data);

    /* Set the SaNrlEn mode.            */
    retVal = hwSetPortRegField(hwPort, QD_REG_INGRESS_RATE_CTRL,14,1,data);
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    DBG_INFO(("OK.\r\r\n"));

    return GT_OK;
}

/*******************************************************************************
* grcGetSaNrlEn
*
* DESCRIPTION:
*       This routine gets SA None Rate Limit (NRL) mode.
*        When SA NRL is enabled and the source address of a frame results in a ATU
*        hit where the SA's MAC address returns an EntryState that indicates Non
*        Rate Limited, then the frame will not be ingress nor egress rate limited.
*
* INPUTS:
*       port - logical port number.
*
* OUTPUTS:
*        mode - GT_TRUE to enable SA None Rate Limit
*               GT_FALSE otherwise
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcGetSaNrlEn
(
    IN  GT_LPORT    port,
    OUT GT_BOOL        *mode
)
{
    GT_U16          data;           
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;        /* Physical port.               */

    DBG_INFO(("grcGetSaNrlEn Called.\r\r\n"));

    hwPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    if (/*!IS_IN_DEV_GROUP(DEV_NONE_RATE_LIMIT)*/1)
    {
           DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }

    /* Get the SaNrlEn mode.            */
    retVal = hwGetPortRegField(hwPort, QD_REG_INGRESS_RATE_CTRL,14,1,&data);
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    BIT_2_BOOL(data, *mode);

    DBG_INFO(("OK.\r\r\n"));

    return GT_OK;
}

/*******************************************************************************
* grcSetDaNrlEn
*
* DESCRIPTION:
*       This routine enables/disables DA None Rate Limit (NRL).
*        When DA NRL is enabled and the destination address of a frame results in 
*        a ATU hit where the DA's MAC address returns an EntryState that indicates 
*        Non Rate Limited, then the frame will not be ingress nor egress rate 
*        limited.
*
* INPUTS:
*       port - logical port number.
*        mode - GT_TRUE to enable DA None Rate Limit
*               GT_FALSE otherwise
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcSetDaNrlEn
(
    IN  GT_QD_DEV    *dev,
    IN  GT_LPORT    port,
    IN  GT_BOOL        mode
)
{
    GT_U16          data;           
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;        /* Physical port.               */

    DBG_INFO(("grcSetDaNrlEn Called.\r\r\n"));

    hwPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_NONE_RATE_LIMIT)*/0)
    {
           DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }

    /* translate BOOL to binary */
    BOOL_2_BIT(mode, data);

    /* Set the DaNrlEn mode.            */
    retVal = hwSetPortRegField(hwPort, QD_REG_INGRESS_RATE_CTRL,13,1,data);
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    DBG_INFO(("OK.\r\r\n"));

    return GT_OK;
}

/*******************************************************************************
* grcGetDaNrlEn
*
* DESCRIPTION:
*       This routine gets SA None Rate Limit (NRL) mode.
*        When DA NRL is enabled and the destination address of a frame results in 
*        a ATU hit where the DA's MAC address returns an EntryState that indicates 
*        Non Rate Limited, then the frame will not be ingress nor egress rate 
*        limited.
*
* INPUTS:
*       port - logical port number.
*
* OUTPUTS:
*        mode - GT_TRUE to enable DA None Rate Limit
*               GT_FALSE otherwise
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcGetDaNrlEn
(
    IN  GT_LPORT    port,
    OUT GT_BOOL        *mode
)
{
    GT_U16          data;           
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;        /* Physical port.               */

    DBG_INFO(("grcGetDaNrlEn Called.\r\r\n"));

    hwPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_NONE_RATE_LIMIT)*/1)
    {
           DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }

    /* Get the DaNrlEn mode.            */
    retVal = hwGetPortRegField(hwPort, QD_REG_INGRESS_RATE_CTRL,13,1,&data);
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    BIT_2_BOOL(data, *mode);

    DBG_INFO(("OK.\r\r\n"));

    return GT_OK;
}


/*******************************************************************************
* grcSetELimitMode
*
* DESCRIPTION:
*       This routine sets Egress Rate Limit counting mode.
*        The supported modes are as follows:
*            GT_PIRL_ELIMIT_FRAME -
*                Count the number of frames
*            GT_PIRL_ELIMIT_LAYER1 -
*                Count all Layer 1 bytes: 
*                Preamble (8bytes) + Frame's DA to CRC + IFG (12bytes)
*            GT_PIRL_ELIMIT_LAYER2 -
*                Count all Layer 2 bytes: Frame's DA to CRC
*            GT_PIRL_ELIMIT_LAYER1 -
*                Count all Layer 1 bytes: 
*                Frame's DA to CRC - 18 - 4 (if frame is tagged)
*
* INPUTS:
*       port - logical port number
*        mode - GT_PIRL_ELIMIT_MODE enum type
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcSetELimitMode
(
    IN  GT_LPORT    port,
    IN  GT_PIRL_ELIMIT_MODE        mode
)
{

    //GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;        /* Physical port.               */

    DBG_INFO(("grcSetELimitMode Called.\r\r\n"));

    hwPort = lport2port(port);

    /* check if the given Switch supports this feature. */
    if (/*!((IS_IN_DEV_GROUP(dev,DEV_PIRL_RESOURCE)) ||
        (IS_IN_DEV_GROUP(dev,DEV_ELIMIT_FRAME_BASED)))*/1)
    {
           DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }
/*
    if (!IS_IN_DEV_GROUP(dev,DEV_ELIMIT_FRAME_BASED))
    {
        if(mode == GT_PIRL_ELIMIT_FRAME)
            return GT_NOT_SUPPORTED;
    }

    data = (GT_U16)mode & 0x3;
*/
    /* Set the Elimit mode.            */
/*    retVal = hwSetPortRegField(dev,hwPort, QD_REG_EGRESS_RATE_CTRL,14,2,data);
    if(retVal != GT_OK)
       {
        DBG_INFO(("Failed.\r\r\n"));
           return retVal;
    }

    DBG_INFO(("OK.\r\r\n"));
*/
    return GT_OK;
}


/*******************************************************************************
* grcGetELimitMode
*
* DESCRIPTION:
*       This routine gets Egress Rate Limit counting mode.
*        The supported modes are as follows:
*            GT_PIRL_ELIMIT_FRAME -
*                Count the number of frames
*            GT_PIRL_ELIMIT_LAYER1 -
*                Count all Layer 1 bytes: 
*                Preamble (8bytes) + Frame's DA to CRC + IFG (12bytes)
*            GT_PIRL_ELIMIT_LAYER2 -
*                Count all Layer 2 bytes: Frame's DA to CRC
*            GT_PIRL_ELIMIT_LAYER1 -
*                Count all Layer 1 bytes: 
*                Frame's DA to CRC - 18 - 4 (if frame is tagged)
*
* INPUTS:
*       port - logical port number
*
* OUTPUTS:
*        mode - GT_PIRL_ELIMIT_MODE enum type
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*        GT_NOT_SUPPORTED    - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcGetELimitMode
(
    IN  GT_LPORT    port,
    OUT GT_PIRL_ELIMIT_MODE        *mode
)
{
    //GT_U16            data;
    //GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;        /* Physical port.               */

    DBG_INFO(("grcGetELimitMode Called.\r\r\n"));

    hwPort = lport2port(port);


    return GT_NOT_SUPPORTED;


    /* Get the Elimit mode.            */
    //retVal = hwGetPortRegField(hwPort, QD_REG_EGRESS_RATE_CTRL,14,2,&data);
    //if(retVal != GT_OK)
    //   {
    //    DBG_INFO(("Failed.\r\r\n"));
    //       return retVal;
    //}

    //*mode = data;

    //DBG_INFO(("OK.\r\r\n"));

    return GT_OK;
}

/*******************************************************************************
* grcSetRsvdNrlEn
*
* DESCRIPTION:
*       This routine sets Reserved Non Rate Limit.
*        When this feature is enabled, frames that match the requirements of the 
*        Rsvd2Cpu bit below will also be considered to be ingress and egress non 
*        rate limited.
*
* INPUTS:
*       en - GT_TRUE to enable Reserved Non Rate Limit,
*             GT_FALSE to disable
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS grcSetRsvdNrlEn
(
    IN  GT_BOOL   en
)
{
    //GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16            data;

    DBG_INFO(("grcSetRsvdNrlEn Called.\r\r\n"));

    if (/*!IS_IN_DEV_GROUP(dev,DEV_NONE_RATE_LIMIT)*/1)
    {
        DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }

    BOOL_2_BIT(en,data);

    /* Set the RsvdNrl bit.            */
 /*   retVal = hwSetGlobalRegField(dev,QD_REG_MANGEMENT_CONTROL,4,1,data);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\r\r\n"));
        return retVal;
    }

    DBG_INFO(("OK.\r\r\n"));
    */
    return GT_OK;

}

/*******************************************************************************
* grcGetRsvdNrlEn
*
* DESCRIPTION:
*       This routine gets Reserved Non Rate Limit.
*        When this feature is enabled, frames that match the requirements of the 
*        Rsvd2Cpu bit below will also be considered to be ingress and egress non 
*        rate limited.
*
* INPUTS:
*       en - GT_TRUE to enable Reserved Non Rate Limit,
*             GT_FALSE to disable
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS grcGetRsvdNrlEn
(
    OUT GT_BOOL   *en
)
{
    //GT_STATUS       retVal;         /* Functions return value.      */
    //GT_U16            data;

    DBG_INFO(("grcGetRsvdNrlEn Called.\r\r\n"));

    if (/*!IS_IN_DEV_GROUP(dev,DEV_NONE_RATE_LIMIT)*/1)
    {
        DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }

    /* Get the RsvdNrl bit.            */
  /*  retVal = hwGetGlobalRegField(dev,QD_REG_MANGEMENT_CONTROL,4,1,&data);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\r\r\n"));
        return retVal;
    }

    BIT_2_BOOL(data, *en);

    DBG_INFO(("OK.\r\r\n"));
    */
    return GT_OK;
}


/*******************************************************************************
* grcSetFrameOverhead
*
* DESCRIPTION:
*       Egress rate frame overhead adjustment.
*        This field is used to adjust the number of bytes that need to be added to a
*        frame's IFG on a per frame basis.
*
*        The egress rate limiter multiplies the value programmed in this field by four
*        for computing the frame byte offset adjustment value (i.e., the amount the
*        IPG is increased for every frame). This adjustment, if enabled, is made to
*        every egressing frame's IPG and it is made in addition to any other IPG
*        adjustments due to other Egress Rate Control settings.
*
*        The egress overhead adjustment can add the following number of byte times
*        to each frame's IPG: 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52,
*        56 and 60.
*
*        Example:
*        If FrameOverhead = 11, the egress rate limiter would increase the IPG
*        between every frame by an additional 44 bytes.
*
*        Note: When the Count Mode (port offset 0x0A) is in Frame based egress rate
*        shaping mode, these Frame Overhead bits must be 0x0.
*
* INPUTS:
*       port     - logical port number.
*       overhead - Frame overhead (0 ~ 15)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_BAD_PARAM        - on bad parameters
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS: 
*
*******************************************************************************/
GT_STATUS grcSetFrameOverhead
(
    IN GT_LPORT            port,
    IN GT_32            overhead
)
{

    //GT_STATUS       retVal;         /* Functions return value.      */
   // GT_U8           phyPort;        /* Physical port.               */

    DBG_INFO(("grcSetFrameOverhead Called.\r\r\n"));


    /* check if device supports this feature */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_ELIMIT_FRAME_BASED)*/1)
    {
        DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }

   /* if (overhead > 15)
    {
        DBG_INFO(("GT_BAD_PARAM \r\r\n"));
        return GT_BAD_PARAM;
    }

    retVal = hwSetPortRegField(dev,phyPort,QD_REG_RATE_CTRL0,8,4,(GT_U16)overhead );
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\r\r\n"));
        return retVal;
    }
    DBG_INFO(("OK.\r\r\n"));*/
    return GT_OK;
}



/*******************************************************************************
* grcGetFrameOverhead
*
* DESCRIPTION:
*       Egress rate frame overhead adjustment.
*        This field is used to adjust the number of bytes that need to be added to a
*        frame's IFG on a per frame basis.
*
*        The egress rate limiter multiplies the value programmed in this field by four
*        for computing the frame byte offset adjustment value (i.e., the amount the
*        IPG is increased for every frame). This adjustment, if enabled, is made to
*        every egressing frame's IPG and it is made in addition to any other IPG
*        adjustments due to other Egress Rate Control settings.
*
*        The egress overhead adjustment can add the following number of byte times
*        to each frame's IPG: 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52,
*        56 and 60.
*
*        Example:
*        If FrameOverhead = 11, the egress rate limiter would increase the IPG
*        between every frame by an additional 44 bytes.
*
*        Note: When the Count Mode (port offset 0x0A) is in Frame based egress rate
*        shaping mode, these Frame Overhead bits must be 0x0.
*
* INPUTS:
*       port    - logical port number.
*
* OUTPUTS:
*       overhead - Frame overhead (0 ~ 15)
*
* RETURNS:
*       GT_OK            - on success
*       GT_FAIL          - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
*******************************************************************************/
GT_STATUS grcGetFrameOverhead
(
    IN  GT_LPORT port,
    OUT GT_32    *overhead
)
{
    //GT_STATUS       retVal;         /* Functions return value.      */
    //GT_U16          data;           /* The register's read data.    */
    //GT_U8           phyPort;        /* Physical port.               */

    DBG_INFO(("grcGetFrameOverhead Called.\r\r\n"));



    /* check if device supports this feature */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_ELIMIT_FRAME_BASED)*/1)
    {
        DBG_INFO(("GT_NOT_SUPPORTED\r\r\n"));
        return GT_NOT_SUPPORTED;
    }
    /*
    retVal = hwGetPortRegField(dev,phyPort,QD_REG_RATE_CTRL0,8,4,&data);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\r\r\n"));
        return retVal;
    }

    *overhead = (GT_U32)data;
    DBG_INFO(("OK.\r\r\n"));
    */
    return GT_OK;
}
#endif

//config rate limit for 88e6240
GT_STATUS PSW1G_RateLimitConfig(void){
u8 port;
GT_PIRL2_DATA pirlData;
GT_STATUS status;
GT_U32    irlRes;
GT_ERATE_TYPE rateType;

	for(uint8_t i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		port = L2F_port_conv(i);

		//egress rate limit
		//make valid rate
		rateType.kbRate = get_rate_limit_tx(i);
		if(rateType.kbRate < 1000){
			rateType.kbRate /=64;
			rateType.kbRate *= 64;
		}else if(rateType.kbRate <= 100000){
			rateType.kbRate /=1000;
			rateType.kbRate *=1000;
		}
		else if(rateType.kbRate <= 1000000)
        {
			rateType.kbRate /= 10000;
			rateType.kbRate *= 10000;
        }
		setEnhancedERate(port,&rateType);


		//ingress rate limit
		status = gpirl2SetCurTimeUpInt(4);
	    switch (status)
	    {
	        case GT_OK:
	            break;
	        case GT_NOT_SUPPORTED:
	        	DEBUG_MSG(RATE_LIM_DBG,"Device is not supporting PIRL.\r\n");
	            return status;
	        default:
	        	DEBUG_MSG(RATE_LIM_DBG,"Failure to configure device.\t\n");
	            return status;
	    }

	    //port = 0;
		irlRes = 0;

		//if(get_rate_limit_rx(i)==0)
		//	pirlData.ingressRate = 2000000;//set max rate
		pirlData.ingressRate = (get_rate_limit_rx(i)*120)/100;

		pirlData.ingressRate /=2;


		if(pirlData.ingressRate < 1000){
			pirlData.ingressRate /=64;
			pirlData.ingressRate *= 64;
		}else if(pirlData.ingressRate <= 100000){
			pirlData.ingressRate /=1000;
			pirlData.ingressRate *=1000;
		}
		else if(pirlData.ingressRate <= 1000000)
        {
			pirlData.ingressRate /= 10000;
			pirlData.ingressRate *= 10000;
        }

		pirlData.customSetup.isValid = GT_FALSE;

		pirlData.accountQConf         = GT_FALSE;
		pirlData.accountFiltered    = GT_TRUE;

		pirlData.mgmtNrlEn = GT_TRUE;
		pirlData.saNrlEn   = GT_TRUE;
		pirlData.daNrlEn   = GT_FALSE;
		pirlData.samplingMode = GT_FALSE;
		pirlData.actionMode = PIRL_ACTION_USE_LIMIT_ACTION;

		pirlData.ebsLimitAction     = ESB_LIMIT_ACTION_DROP;
		pirlData.bktRateType        = BUCKET_TYPE_RATE_BASED;

		/*pirlData.bktTypeMask        = BUCKET_TRAFFIC_BROADCAST |
									  BUCKET_TRAFFIC_MULTICAST |
									  BUCKET_TRAFFIC_UNICAST   |
									  BUCKET_TRAFFIC_MGMT_FRAME|
									  BUCKET_TRAFFIC_ARP;*/

		pirlData.priORpt = GT_TRUE;
		pirlData.priMask = 0;

		pirlData.byteTobeCounted    = GT_PIRL2_COUNT_ALL_LAYER2;

		status = gpirl2WriteResource(port,irlRes,&pirlData);

	    switch (status)
	    {
	        case GT_OK:
	        	DEBUG_MSG(RATE_LIM_DBG,"PIRL2 writing completed.\r\n");
	            break;
	        case GT_BAD_PARAM:
	        	DEBUG_MSG(RATE_LIM_DBG,"Invalid parameters are given.\r\n");
	            break;
	        case GT_NOT_SUPPORTED:
	        	DEBUG_MSG(RATE_LIM_DBG,"Device is not supporting PIRL2.\r\n");
	            break;
	        default:
	        	DEBUG_MSG(RATE_LIM_DBG,"Failure to configure device.\r\n");
	            break;
	    }
	}
	return GT_OK;
}

/*******************************************************************************
* gpirl2SetCurTimeUpInt
*
* DESCRIPTION:
*       This function sets the current time update interval.
*        Please contact FAE for detailed information.
*
* INPUTS:
*       upInt - updata interval (0 ~ 7)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*        GT_BAD_PARAM - if invalid parameter is given
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gpirl2SetCurTimeUpInt
(
    GT_U32                upInt
)
{
    GT_STATUS       retVal;        /* Functions return value */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA    opData;

    /* check if device supports this feature */
    /*if (!IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }*/

    if (upInt > 0x7)
        return GT_BAD_PARAM;

    op = PIRL_READ_RESOURCE;

    opData.irlPort = 0xF;
    opData.irlRes = 0;
    opData.irlReg = 1;
    opData.irlData = 0;

    retVal = pirl2OperationPerform(op, &opData);
    if (retVal != GT_OK)
    {
    		DEBUG_MSG(RATE_LIM_DBG,"PIRL OP Failed.\r\n");
            return retVal;
    }

    op = PIRL_WRITE_RESOURCE;
    opData.irlData = (opData.irlData & 0xFFF8) | (GT_U16)upInt;

    retVal = pirl2OperationPerform(op, &opData);
    if (retVal != GT_OK)
    {
    	   DEBUG_MSG(RATE_LIM_DBG,"PIRL OP Failed.\r\n");
           return retVal;
    }

    return GT_OK;
}


/*******************************************************************************
* pirl2OperationPerform
*
* DESCRIPTION:
*       This function accesses Ingress Rate Command Register and Data Register.
*
* INPUTS:
*       pirlOp     - The stats operation bits to be written into the stats
*                    operation register.
*
* OUTPUTS:
*       pirlData   - points to the data storage where the MIB counter will be saved.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS pirl2OperationPerform
(
    GT_PIRL2_OPERATION    pirlOp,
    GT_PIRL2_OP_DATA        *opData
)
{
    GT_STATUS       retVal;    /* Functions return value */
    GT_U16          data;     /* temporary Data storage */

    /* Wait until the pirl in ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_INGRESS_RATE_COMMAND;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->pirlRegsSem);
        return retVal;
      }
    }
#else
    data = 1;
    while(data == 1)
    {
        retVal = hwGetGlobal2RegField(QD_REG_INGRESS_RATE_COMMAND,15,1,&data);
        if(retVal != GT_OK)
        {
            return retVal;
        }
    }
#endif

    /* Set the PIRL Operation register */
    switch (pirlOp)
    {
        case PIRL_INIT_ALL_RESOURCE:
            data = (1 << 15) | (PIRL_INIT_ALL_RESOURCE << 12);
            retVal = hwWriteGlobal2Reg(QD_REG_INGRESS_RATE_COMMAND,data);
            if(retVal != GT_OK)
            {
                return retVal;
            }
            break;
        case PIRL_INIT_RESOURCE:
            data = (GT_U16)((1 << 15) | (PIRL_INIT_RESOURCE << 12) |
                    (opData->irlPort << 8) |
                    (opData->irlRes << 5));
            retVal = hwWriteGlobal2Reg(QD_REG_INGRESS_RATE_COMMAND,data);
            if(retVal != GT_OK)
            {
                 return retVal;
            }
            break;

        case PIRL_WRITE_RESOURCE:
            data = (GT_U16)opData->irlData;
            retVal = hwWriteGlobal2Reg(QD_REG_INGRESS_RATE_DATA,data);
            if(retVal != GT_OK)
            {
                 return retVal;
            }

            data = (GT_U16)((1 << 15) | (PIRL_WRITE_RESOURCE << 12) |
                    (opData->irlPort << 8)    |
                    (opData->irlRes << 5)    |
                    (opData->irlReg & 0xF));
            retVal = hwWriteGlobal2Reg(QD_REG_INGRESS_RATE_COMMAND,data);
            if(retVal != GT_OK)
            {
                return retVal;
            }
            break;

        case PIRL_READ_RESOURCE:
            data = (GT_U16)((1 << 15) | (PIRL_READ_RESOURCE << 12) |
                    (opData->irlPort << 8)    |
                    (opData->irlRes << 5)    |
                    (opData->irlReg & 0xF));
            retVal = hwWriteGlobal2Reg(QD_REG_INGRESS_RATE_COMMAND,data);
            if(retVal != GT_OK)
            {
                return retVal;
            }

#ifdef GT_RMGMT_ACCESS
            {
              HW_DEV_REG_ACCESS regAccess;

              regAccess.entries = 1;

              regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
              regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
              regAccess.rw_reg_list[0].reg = QD_REG_INGRESS_RATE_COMMAND;
              regAccess.rw_reg_list[0].data = 15;
              retVal = hwAccessMultiRegs(dev, &regAccess);
              if(retVal != GT_OK)
              {
                gtSemGive(dev,dev->pirlRegsSem);
                return retVal;
              }
            }
#else
            data = 1;
            while(data == 1)
            {
                retVal = hwGetGlobal2RegField(QD_REG_INGRESS_RATE_COMMAND,15,1,&data);
                if(retVal != GT_OK)
                {
                    return retVal;
                }
            }
#endif

            retVal = hwReadGlobal2Reg(QD_REG_INGRESS_RATE_DATA,&data);
            opData->irlData = (GT_U32)data;
            if(retVal != GT_OK)
            {
                return retVal;
            }
            return retVal;

        default:
            return GT_FAIL;
    }

    /* Wait until the pirl in ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;
      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_INGRESS_RATE_COMMAND;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->pirlRegsSem);
        return retVal;
      }
    }
#else
    data = 1;
    while(data == 1)
    {
        retVal = hwGetGlobal2RegField(QD_REG_INGRESS_RATE_COMMAND,15,1,&data);
        if(retVal != GT_OK)
        {
            return retVal;
        }
    }
#endif
    return retVal;
}


/*******************************************************************************
* gpirl2WriteResource
*
* DESCRIPTION:
*        This routine writes resource bucket parameters to the given resource
*        of the port.
*
* INPUTS:
*        port     - logical port number.
*        irlRes   - bucket to be used (0 ~ 4).
*        pirlData - PIRL resource parameters.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if invalid parameter is given
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gpirl2WriteResource
(
    GT_LPORT    port,
    GT_U32        irlRes,
    GT_PIRL2_DATA    *pirlData
)
{
    GT_STATUS           retVal;
    GT_PIRL2_RESOURCE    pirlRes;
    GT_U32               irlPort;         /* the physical port number     */
    GT_U32                maxRes;

    DEBUG_MSG(RATE_LIM_DBG,"gpirl2WriteResource Called.\r\n");

    /* check if device supports this feature */
    /*if (!IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }*/

    /* check if the given bucket number is valid */
    if (/*IS_IN_DEV_GROUP(dev,DEV_RESTRICTED_PIRL2_RESOURCE)*/0)
    {
        maxRes = 2;
    }
    else
    {
        maxRes = 5;
    }

    if (irlRes >= maxRes)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM irlRes\r\n");
        return GT_BAD_PARAM;
    }

    //irlPort = (GT_U32)GT_LPORT_2_PORT(port);
    irlPort = (GT_U32)lport2port(port);
    if (irlPort == GT_INVALID_PORT)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM port\r\n");
        return GT_BAD_PARAM;
    }

    /* Initialize internal counters */
    retVal = pirl2InitIRLResource(irlPort,irlRes);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"PIRL Write Resource failed.\r\n");
        return retVal;
    }

    /* Program the Ingress Rate Resource Parameters */
    retVal = pirl2DataToResource(pirlData,&pirlRes);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"PIRL Data to PIRL Resource conversion failed.\r\n");
        return retVal;
    }

    retVal = pirl2WriteResource(irlPort,irlRes,&pirlRes);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"PIRL Write Resource failed.\r\n");
        return retVal;
    }

    DEBUG_MSG(RATE_LIM_DBG,"OK.\r\n");
    return GT_OK;
}


/*
 * Initialize the selected PIRL resource to the inital state.
 * This function initializes only the BSM structure for the IRL Unit.
*/
static GT_STATUS pirl2InitIRLResource
(
    GT_U32                irlPort,
    GT_U32                irlRes
)
{
    GT_STATUS       	retVal;    /* Functions return value */
    GT_PIRL2_OPERATION  op;
    GT_PIRL2_OP_DATA    opData;

    op = PIRL_INIT_RESOURCE;
    opData.irlPort = irlPort;
    opData.irlRes = irlRes;

    retVal = pirl2OperationPerform(op, &opData);
    if (retVal != GT_OK)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"PIRL OP Failed.\r\n");
        return retVal;
    }

    return retVal;
}


/*
 * convert PIRL Data structure to PIRL Resource structure.
 * if PIRL Data is not valid, return GT_BAD_PARARM;
*/
static GT_STATUS pirl2DataToResource
(
    GT_PIRL2_DATA        *pirlData,
    GT_PIRL2_RESOURCE    *res
)
{
    GT_U32 typeMask;
    GT_U32 data;

    gtMemSet((void*)res,0,sizeof(GT_PIRL2_RESOURCE));

    data = (GT_U32)(pirlData->accountQConf|pirlData->accountFiltered|
                    pirlData->mgmtNrlEn|pirlData->saNrlEn|pirlData->daNrlEn|
                    pirlData->samplingMode);

    if (data > 1)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM (Boolean)\r\n");
        return GT_BAD_PARAM;
    }

    /*if (IS_IN_DEV_GROUP(dev,DEV_RESTRICTED_PIRL2_RESOURCE))
    {
        if (pirlData->samplingMode != GT_FALSE)
        {
        	DEBUG_MSG(DEBUG_QD,"GT_BAD_PARAM (sampling mode)\r\n");
            return GT_BAD_PARAM;
        }
    }*/

    res->accountQConf = pirlData->accountQConf;
    res->accountFiltered = pirlData->accountFiltered;
    res->mgmtNrlEn = pirlData->mgmtNrlEn;
    res->saNrlEn = pirlData->saNrlEn;
    res->daNrlEn = pirlData->daNrlEn;
    res->samplingMode = pirlData->samplingMode;

    switch(pirlData->actionMode)
    {
        case PIRL_ACTION_ACCEPT:
        case PIRL_ACTION_USE_LIMIT_ACTION:
            res->actionMode = pirlData->actionMode;
            break;
        default:
        	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM actionMode\r\n");
            return GT_BAD_PARAM;
    }

    switch(pirlData->ebsLimitAction)
    {
        case ESB_LIMIT_ACTION_DROP:
        case ESB_LIMIT_ACTION_FC:
            res->ebsLimitAction = pirlData->ebsLimitAction;
            break;
        default:
        	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM ebsLimitAction\r\n");
            return GT_BAD_PARAM;
    }

    switch(pirlData->fcDeassertMode)
    {
        case GT_PIRL_FC_DEASSERT_EMPTY:
        case GT_PIRL_FC_DEASSERT_CBS_LIMIT:
            res->fcDeassertMode = pirlData->fcDeassertMode;
            break;
        default:
            if(res->ebsLimitAction != ESB_LIMIT_ACTION_FC)
            {
                res->fcDeassertMode    = GT_PIRL_FC_DEASSERT_EMPTY;
                break;
            }
            DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM fcDeassertMode\r\n");
            return GT_BAD_PARAM;
    }

    if(pirlData->customSetup.isValid == GT_TRUE)
    {
        res->ebsLimit = pirlData->customSetup.ebsLimit;
        res->cbsLimit = pirlData->customSetup.cbsLimit;
        res->bktIncrement = pirlData->customSetup.bktIncrement;
        res->bktRateFactor = pirlData->customSetup.bktRateFactor;
    }
    else
    {
        if(pirlData->ingressRate == 0)
        {
        	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM ingressRate(%lu)\r\n",pirlData->ingressRate);
            return GT_BAD_PARAM;
        }

        if(pirlData->ingressRate < 1000)    /* less than 1Mbps */
        {
            /* it should be divided by 64 */
            if(pirlData->ingressRate % 64)
            {
            	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM ingressRate(%lu)\t\n",pirlData->ingressRate);
                return GT_BAD_PARAM;
            }
            res->bktRateFactor = pirlData->ingressRate/64;
        }
        else if(pirlData->ingressRate < 10000)    /* less than or equal to 10Mbps */
        {
            /* it should be divided by 1000 */
            if(pirlData->ingressRate % 1000)
            {
            	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM ingressRate(%lu)\r\n",pirlData->ingressRate);
                return GT_BAD_PARAM;
            }
            res->bktRateFactor = pirlData->ingressRate/128 + ((pirlData->ingressRate % 128)?1:0);
        }
        else if(pirlData->ingressRate < 100000)    /* less than or equal to 100Mbps */
        {
            /* it should be divided by 1000 */
            if(pirlData->ingressRate % 1000)
            {
            	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM ingressRate(%lu)\r\n",pirlData->ingressRate);
                return GT_BAD_PARAM;
            }
            res->bktRateFactor = pirlData->ingressRate/1000;
        }
        else if(pirlData->ingressRate <= 200000)    /* less than or equal to 200Mbps */
        {
            /* it should be divided by 10000 */
            if(pirlData->ingressRate % 10000)
            {
            	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM ingressRate(%lu)\r\n",pirlData->ingressRate);
                return GT_BAD_PARAM;
            }
            res->bktRateFactor = pirlData->ingressRate/1000;
        }
        else
        {
        	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM ingressRate(%lu)\r\n",pirlData->ingressRate);
            return GT_BAD_PARAM;
        }

        res->ebsLimit = RECOMMENDED_ESB_LIMIT(pirlData->ingressRate);
        res->cbsLimit = RECOMMENDED_CBS_LIMIT(pirlData->ingressRate);
        res->bktIncrement = RECOMMENDED_BUCKET_INCREMENT(pirlData->ingressRate);
    }

    switch(pirlData->bktRateType)
    {
        case BUCKET_TYPE_TRAFFIC_BASED:
            res->bktRateType = pirlData->bktRateType;

            typeMask = 0x7FFF;

            if (pirlData->bktTypeMask > typeMask)
            {
            	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM bktTypeMask(%#lx)\r\n",pirlData->bktTypeMask);
                return GT_BAD_PARAM;
            }

               res->bktTypeMask = pirlData->bktTypeMask;

            if (pirlData->bktTypeMask & BUCKET_TRAFFIC_ARP)
            {
                res->bktTypeMask &= ~BUCKET_TRAFFIC_ARP;
                res->bktTypeMask |= 0x80;
            }

            if (pirlData->priORpt > 1)
            {
            	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM rpiORpt\r\n");
                return GT_BAD_PARAM;
            }

            res->priORpt = pirlData->priORpt;

            if (pirlData->priMask >= (1 << 4))
            {
            	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM priMask(%#lx)\r\n",pirlData->priMask);
                return GT_BAD_PARAM;
            }

            res->priMask = pirlData->priMask;

            break;

        case BUCKET_TYPE_RATE_BASED:
            res->bktRateType = pirlData->bktRateType;
               res->bktTypeMask = pirlData->bktTypeMask;
            res->priORpt = pirlData->priORpt;
            res->priMask = pirlData->priMask;
            break;

        default:
        	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM bktRateType(%#x)\r\n",pirlData->bktRateType);
            return GT_BAD_PARAM;
    }

    switch(pirlData->byteTobeCounted)
    {
        case GT_PIRL2_COUNT_FRAME:
        case GT_PIRL2_COUNT_ALL_LAYER1:
        case GT_PIRL2_COUNT_ALL_LAYER2:
        case GT_PIRL2_COUNT_ALL_LAYER3:
            res->byteTobeCounted = pirlData->byteTobeCounted;
            break;
        default:
        	DEBUG_MSG(RATE_LIM_DBG,"GT_BAD_PARAM byteTobeCounted(%#x)\r\n",pirlData->byteTobeCounted);
            return GT_BAD_PARAM;
    }

    return GT_OK;
}


/*******************************************************************************
* pirl2WriteResource
*
* DESCRIPTION:
*       This function writes IRL Resource to BCM (Bucket Configuration Memory)
*
* INPUTS:
*       irlPort - physical port number.
*        irlRes  - bucket to be used (0 ~ 4).
*       res     - IRL Resource data
*
* OUTPUTS:
*       Nont.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS pirl2WriteResource
(
    GT_U32                irlPort,
    GT_U32                irlRes,
    GT_PIRL2_RESOURCE    *res
)
{
    GT_STATUS       retVal;            /* Functions return value */
    GT_U16          data[8];     /* temporary Data storage */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA     opData;
    int                i;

    op = PIRL_WRITE_RESOURCE;

    /* reg0 data */
    data[0] = (GT_U16)((res->bktRateType << 15) |    /* Bit[15] : Bucket Rate Type */
                      (res->bktTypeMask << 0 ));         /* Bit[14:0] : Traffic Type   */

    /* reg1 data */
    data[1] = (GT_U16)res->bktIncrement;    /* Bit[11:0] : Bucket Increment */

    /* reg2 data */
    data[2] = (GT_U16)res->bktRateFactor;    /* Bit[15:0] : Bucket Rate Factor */

    /* reg3 data */
    data[3] = (GT_U16)((res->cbsLimit & 0xFFF) << 4)|    /* Bit[15:4] : CBS Limit[11:0] */
                    (res->byteTobeCounted << 2);        /* Bit[3:0] : Bytes to be counted */

    /* reg4 data */
    data[4] = (GT_U16)(res->cbsLimit >> 12);        /* Bit[11:0] : CBS Limit[23:12] */

    /* reg5 data */
    data[5] = (GT_U16)(res->ebsLimit & 0xFFFF);        /* Bit[15:0] : EBS Limit[15:0] */

    /* reg6 data */
    data[6] = (GT_U16)((res->ebsLimit >> 16)    |    /* Bit[7:0] : EBS Limit[23:16] */
                    (res->samplingMode << 11)    |    /* Bit[11] : Sampling Mode */
                    (res->ebsLimitAction << 12)    |    /* Bit[12] : EBS Limit Action */
                    (res->actionMode << 13)        |    /* Bit[13] : Action Mode */
                    (res->fcDeassertMode << 14));    /* Bit[14] : Flow control mode */

    /* reg7 data */
    data[7] = (GT_U16)((res->daNrlEn)            |    /* Bit[0]  : DA Nrl En */
                    (res->saNrlEn << 1)            |    /* Bit[1]  : SA Nrl En */
                    (res->mgmtNrlEn << 2)         |    /* Bit[2]  : MGMT Nrl En */
                    (res->priMask << 8)         |    /* Bit[11:8] : Priority Queue Mask */
                    (res->priORpt << 12)         |    /* Bit[12] : Priority OR PacketType */
                    (res->accountFiltered << 14)|    /* Bit[14] : Account Filtered */
                    (res->accountQConf << 15));        /* Bit[15] : Account QConf */

    for(i=0; i<8; i++)
    {
        opData.irlPort = irlPort;
        opData.irlRes = irlRes;
        opData.irlReg = i;
        opData.irlData = data[i];

        retVal = pirl2OperationPerform(op, &opData);
        if (retVal != GT_OK)
        {
        	DEBUG_MSG(RATE_LIM_DBG,"PIRL OP Failed.\r\n");
            return retVal;
        }
    }
    return GT_OK;
}


static GT_STATUS setEnhancedERate(GT_LPORT port, GT_ERATE_TYPE *rateType)
{
    GT_STATUS    retVal;         /* Functions return value.      */
    GT_U16        data;
    GT_U32        rate, eDec;
    GT_PIRL_ELIMIT_MODE        mode;
    GT_U8        phyPort;        /* Physical port.               */

    phyPort = lport2port(port);

    mode = GT_PIRL_ELIMIT_LAYER2;//default

    //set mode
    retVal = hwSetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,14,2,(GT_U16)mode);

    if (mode == GT_PIRL_ELIMIT_FRAME)
    {
        /* Count Per Frame */
        rate = rateType->fRate;

        if (rate == 0) /* disable egress rate limit */
        {
            eDec = 0;
            data = 0;
        }
        else if((rate < 7600)  || (rate > 1488000))
        {
            return GT_BAD_PARAM;
        }
        else
        {
            eDec = 1;
            data = (GT_U16)GT_GET_RATE_LIMIT_PER_FRAME(rate,eDec);
        }
    }
    else
    {
        /* Count Per Byte */
        rate = rateType->kbRate;

        if(rate == 0)
        {
            eDec = 0;
        }
        else if(rate < 1000)    /* less than 1Mbps */
        {
            /* it should be divided by 64 */
            if(rate % 64)
                return GT_BAD_PARAM;
            eDec = rate/64;
        }
        else if(rate <= 100000)    /* less than or equal to 100Mbps */
        {
            /* it should be divided by 1000 */
            if(rate % 1000)
                return GT_BAD_PARAM;
            eDec = rate/1000;
        }
        else if(rate <= 1000000)    /* less than or equal to 1000Mbps */
        {
            /* it should be divided by 10000 */
            if(rate % 10000)
                return GT_BAD_PARAM;
            eDec = rate/10000;
        }
        else
            return GT_BAD_PARAM;

        if(rate == 0)
        {
            data = 0;
        }
        else
        {
            data = (GT_U16)GT_GET_RATE_LIMIT_PER_BYTE(rate,eDec);
        }
    }

    retVal = hwSetPortRegField(phyPort,QD_REG_RATE_CTRL0,0,7,(GT_U16)eDec);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"Failed.\r\n");
        return retVal;
    }

    retVal = hwSetPortRegField(phyPort,QD_REG_EGRESS_RATE_CTRL,0,12,(GT_U16)data);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(RATE_LIM_DBG,"Failed.\r\n");
        return retVal;
    }

    DEBUG_MSG(RATE_LIM_DBG,"OK.\r\n");
    return GT_OK;
}

void SWU_RateLimitConfig(void){
u8 port;
//u32 temp;

	//unknown unicast rate limit
	if(get_uc_rate_limit()){
		Salsa2_WriteRegField(UNKNOWN_RATE_LIMIT,0,1,1);
		Salsa2_WriteRegField(UNKNOWN_RATE_LIMIT,2,get_bc_limit()*10,16);//число пакетов
		Salsa2_WriteRegField(UNKNOWN_RATE_LIMIT_WIND,0,1000,16);//ширина окна
	}
	else{
		Salsa2_WriteRegField(UNKNOWN_RATE_LIMIT,0,0,1);
	}


	if(get_mc_rate_limit() || get_bc_rate_limit()){
		Salsa2_WriteRegField(INGRESS_CTRL_REG,20,1,1);//Granularity 1 packet
		Salsa2_WriteRegField(BROADCAST_RL_CTRL,0,((get_bc_limit()*10)/29),16);//n*10 packets counted of 1000pkt window
		Salsa2_WriteRegField(BROADCAST_RL_WIND_REG,0,0x13,16);//Windows size = 1000(512+8+12)*8/1G=4256us /228
		for(uint8_t i=0;i<(ALL_PORT_NUM);i++){
			port = L2F_port_conv(i);
			Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+port*0x1000,11,1,1);//enable BC limit
		}

		if(get_mc_rate_limit())
			Salsa2_WriteRegField(INGRESS_CTRL_REG,29,1,1);//Enable MC Rate Limit
	}
	else{
		Salsa2_WriteRegField(INGRESS_CTRL_REG,29,0,1);//Disable MC Rate Limit
		for(uint8_t i=0;i<(ALL_PORT_NUM);i++){
			port = L2F_port_conv(i);
			Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+port*0x1000,11,0,1);//disable BC limit
		}
	}


}




