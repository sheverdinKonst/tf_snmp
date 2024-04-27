/*
 * FlowCtrl.h
 *
 *  Created on: 06.07.2012
 *      Author: Belyaev
 */

#ifndef FLOWCTRL_H_
#define FLOWCTRL_H_

#include "SpeedDuplex.h"


GT_STATUS gprtSetPause(GT_LPORT  port,GT_PHY_PAUSE_MODE state);
GT_STATUS PortConfig(port_sett_t *port_sett);
u8 PSW1G_PortConfig(port_sett_t *port_sett);
GT_STATUS hwGetGlobalRegField(    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    OUT GT_U16   *data
);
uint8_t portToSmiMapping
(
		uint8_t    portNum,
		uint32_t    accessType
);
GT_STATUS hwSetPortRegField
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
);

GT_STATUS hwSetPortRegField6240
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    pageNum,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
);

GT_STATUS hwGetPortRegField
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    OUT GT_U16   *data
);

GT_STATUS hwSetPhyRegField
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
);

GT_STATUS hwSetPhyRegField6240
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    pageNum,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
);
#endif /* FLOWCTRL_H_ */
