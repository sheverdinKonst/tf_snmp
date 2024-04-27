#ifndef MII_SOFT_H_
#define MII_SOFT_H_


void mii_init(void);
u16 mii_read(u8 phyaddr,u8 reg);
u8 mii_write(u8 phyaddr,u8 reg,u16 data);


#endif /* MII_SOFT_H_ */
