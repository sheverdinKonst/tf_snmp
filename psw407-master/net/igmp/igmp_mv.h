#ifndef IGMP_MV_H_
#define IGMP_MV_H_

#include "msApiTypes.h"
#include "msApiDefs.h"

#define GT_LPORTVEC_2_PORTVEC(_lvec)      (GT_U32)((_lvec) & 0xffff)
#define GT_PORTVEC_2_LPORTVEC(_pvec)       (GT_U32)((_pvec) & 0xffff)

GT_STATUS gfdbAddMacEntry(GT_ATU_ENTRY *macEntry);
GT_STATUS gfdbDelMacEntry(GT_ETHERADDR  *macAddress);
GT_STATUS sampleAddMulticastAddr(u8 *mac, u8 *ports, u8 add_cpu);
GT_STATUS sampleDelMulticastAddr(u8 *mac,u8 *ports,u8 del_cpu);
void add_broadcast_to_atu(void);
GT_STATUS atuOperationPerform(GT_ATU_OPERATION atuOp,  GT_EXTRA_OP_DATA *opData, GT_ATU_ENTRY  *entry);
#endif /* IGMP_MV_H_ */
