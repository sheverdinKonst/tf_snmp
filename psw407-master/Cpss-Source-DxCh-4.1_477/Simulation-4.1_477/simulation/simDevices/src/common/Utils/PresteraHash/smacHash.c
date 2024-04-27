/*******************************************************************************
*                   Copyright 2002, GALILEO TECHNOLOGY, LTD.
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
*
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES,
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.
* (MJKK), GALILEO TECHNOLOGY LTD. (GTL) AND GALILEO TECHNOLOGY, INC.(GTI).
********************************************************************************
* smacHash.c
*
* DESCRIPTION:
*       Hash calculate for MAC address table implementation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 9 $
*
*******************************************************************************/
#include <common/Utils/PresteraHash/smacHash.h>

/*******************************************************************************
* macFunctionGetBit
*
* DESCRIPTION:
*       The function returns the value of a sfecific bit in a word.
*
* INPUTS:
*       word - the word.
*       bit  - the bit number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       the value of the bit.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_U32 macFunctionGetBit
(
    IN GT_U32 word ,
    IN GT_32 bit
)
{
    return ( ( 1 << bit ) & word ) >> bit ;
}


/*******************************************************************************
* macFunctionGetBits
*
* DESCRIPTION:
*       The function returns the value of a sfecific bits in a word.
*
* INPUTS:
*       word   - the word.
*       offset - the first bit number.
*       length - the bits number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       the value of the bits.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_U32 macFunctionGetBits
(
    IN GT_U32 word ,
    IN GT_32 offset ,
    IN GT_32 length
)
{
    GT_U32 num = 0 ;
    GT_32 i;

    for (i= length-1 ; i >= 0 ; i-- )
        num = ( num << 1) + macFunctionGetBit(word, offset + i );
    return num ;
}

/*******************************************************************************
* macFunctionSetBit
*
* DESCRIPTION:
*       The function set a sfecific bit in a word and return the new word.
*
* INPUTS:
*       word  - the word.
*       bit   - the bit number.
*       value - the new value of the bit.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       the new value of the word.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_U32 *macFunctionSetBit
(
    INOUT GT_U32 *word,
    IN    GT_32 bit,
    IN    GT_32 value
)
{

    if (value)
        *word |= ( value << bit );
    else
        *word &= (( 1  <<  bit ) ^ 0xffffffff );

    return word;
}

/*******************************************************************************
* gtMacHashMode0
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address and VLAN id, in mode 0.
*
* INPUTS:
*       addrLow     - the two lower bytes of the mac address.
*       addrHigh    - the four higher bytes of the mac address.
*       vid         - the VLAN id.
*
* OUTPUTS:
*       hash - the hash index.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS gtMacHashMode0
(
    IN  GT_U32          addrLow,
    IN  GT_U32          addrHigh,
    IN  GT_U32          vid,
    OUT GT_U32          *hash
)
{
    GT_U32  tmpVal =0;
    GT_U32  tmpHash =0;

    tmpVal = macFunctionGetBit(addrLow,7)^macFunctionGetBit(addrLow,18)^
        macFunctionGetBit(addrLow,24)^macFunctionGetBit(addrLow,30)^
        macFunctionGetBit(vid,6);
    macFunctionSetBit(&tmpHash,0,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,6)^macFunctionGetBit(addrLow,17)^
        macFunctionGetBit(addrLow,23)^macFunctionGetBit(addrLow,29)^
        macFunctionGetBit(vid,5)^macFunctionGetBit(vid,11);
    macFunctionSetBit(&tmpHash,1,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,9)^macFunctionGetBit(addrLow,13)^
            macFunctionGetBit(addrLow,19)^macFunctionGetBit(addrLow,25)^
            macFunctionGetBit(vid,1)^macFunctionGetBit(vid,7);
    macFunctionSetBit(&tmpHash,2,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,5)^macFunctionGetBit(addrLow,31);
    macFunctionSetBit(&tmpHash,3,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,4)^macFunctionGetBit(addrHigh,0);
    macFunctionSetBit(&tmpHash,4,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,3)^macFunctionGetBit(addrHigh,1);
    macFunctionSetBit(&tmpHash,5,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,2)^macFunctionGetBit(addrHigh,2);
    macFunctionSetBit(&tmpHash,6,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,1)^macFunctionGetBit(addrHigh,3);
    macFunctionSetBit(&tmpHash,7,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,0)^macFunctionGetBit(addrHigh,4);
    macFunctionSetBit(&tmpHash,8,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,8)^macFunctionGetBit(addrHigh,5);
    macFunctionSetBit(&tmpHash,9,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,10)^macFunctionGetBit(addrLow,14)^
            macFunctionGetBit(addrLow,20)^macFunctionGetBit(addrLow,26)^
            macFunctionGetBit(vid,2)^macFunctionGetBit(vid,8);
    macFunctionSetBit(&tmpHash,10,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,11)^macFunctionGetBit(addrLow,15)^
            macFunctionGetBit(addrLow,21)^macFunctionGetBit(addrLow,27)^
            macFunctionGetBit(vid,3)^macFunctionGetBit(vid,9);
    macFunctionSetBit(&tmpHash,11,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,12)^macFunctionGetBit(addrLow,16)^
            macFunctionGetBit(addrLow,22)^macFunctionGetBit(addrLow,28)^
            macFunctionGetBit(vid,0)^macFunctionGetBit(vid,4)^
            macFunctionGetBit(vid,10);
    macFunctionSetBit(&tmpHash,12,tmpVal);

    *hash = tmpHash;
    return GT_OK;
}

/*******************************************************************************
* gtMacHashMode1
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address and VLAN id, in mode 1.
*
* INPUTS:
*       addrLow     - the two lower bytes of the mac address.
*       addrHigh    - the four higher bytes of the mac address.
*       vid         - the VLAN id.
*
* OUTPUTS:
*       hash - the hash index.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS gtMacHashMode1
(
    IN  GT_U32          addrLow,
    IN  GT_U32          addrHigh,
    IN  GT_U32          vid,
    OUT GT_U32          *hash
)
{
    GT_U32  tmpVal =0;
    GT_U32  tmpHash =0;

    tmpVal = macFunctionGetBit(addrLow,0)^macFunctionGetBit(addrLow,13)^
        macFunctionGetBit(addrLow,19)^macFunctionGetBit(addrLow,25)^
        macFunctionGetBit(vid,1)^macFunctionGetBit(vid,7);
    macFunctionSetBit(&tmpHash,0,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,1)^macFunctionGetBit(addrLow,14)^
        macFunctionGetBit(addrLow,20)^macFunctionGetBit(addrLow,26)^
        macFunctionGetBit(vid,2)^macFunctionGetBit(vid,8);
    macFunctionSetBit(&tmpHash,1,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,2)^macFunctionGetBit(addrLow,15)^
            macFunctionGetBit(addrLow,21)^macFunctionGetBit(addrLow,27)^
            macFunctionGetBit(vid,3)^macFunctionGetBit(vid,9);
    macFunctionSetBit(&tmpHash,2,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,3)^macFunctionGetBit(addrLow,16)^
            macFunctionGetBit(addrLow,22)^macFunctionGetBit(addrLow,28)^
            macFunctionGetBit(vid,4)^macFunctionGetBit(vid,10);
    macFunctionSetBit(&tmpHash,3,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,4)^macFunctionGetBit(addrLow,17)^
            macFunctionGetBit(addrLow,23)^macFunctionGetBit(addrLow,29)^
            macFunctionGetBit(vid,5)^macFunctionGetBit(vid,11);
    macFunctionSetBit(&tmpHash,4,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,5)^macFunctionGetBit(addrLow,18)^
            macFunctionGetBit(addrLow,24)^macFunctionGetBit(addrLow,30)^
            macFunctionGetBit(vid,6);
    macFunctionSetBit(&tmpHash,5,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,6)^macFunctionGetBit(addrHigh,0);
    macFunctionSetBit(&tmpHash,6,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,7)^macFunctionGetBit(addrHigh,1);
    macFunctionSetBit(&tmpHash,7,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,8)^macFunctionGetBit(addrHigh,2);
    macFunctionSetBit(&tmpHash,8,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,9)^macFunctionGetBit(addrHigh,3);
    macFunctionSetBit(&tmpHash,9,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,10)^macFunctionGetBit(addrHigh,4);
    macFunctionSetBit(&tmpHash,10,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,11)^macFunctionGetBit(addrHigh,5);
    macFunctionSetBit(&tmpHash,11,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,12)^macFunctionGetBit(addrHigh,6)^
        macFunctionGetBit(vid,0);
    macFunctionSetBit(&tmpHash,12,tmpVal);

    *hash = tmpHash;
    return GT_OK;
}

/*******************************************************************************
* gtMacHashMode1Fixed
*
* DESCRIPTION:
*       This function calculates the fixed hash index for the mac address table.
*       for specific mac address and VLAN id, in mode 1.
*
* INPUTS:
*       addrLow     - the two lower bytes of the mac address.
*       addrHigh    - the four higher bytes of the mac address.
*       vid         - the VLAN id.
*
* OUTPUTS:
*       hash - the hash index.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS gtMacHashMode1Fixed
(
    IN  GT_U32          addrLow,
    IN  GT_U32          addrHigh,
    IN  GT_U32          vid,
    OUT GT_U32          *hash
)
{
    GT_U32  tmpVal =0;
    GT_U32  tmpHash =0;

    tmpVal = macFunctionGetBit(addrLow,0)^macFunctionGetBit(addrLow,13)^
        macFunctionGetBit(addrLow,26)^macFunctionGetBit(addrHigh,7)^
        macFunctionGetBit(vid,1)^macFunctionGetBit(vid,7);
    macFunctionSetBit(&tmpHash,12,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,1)^macFunctionGetBit(addrLow,14)^
        macFunctionGetBit(addrLow,27)^macFunctionGetBit(addrHigh,8)^
        macFunctionGetBit(vid,2)^macFunctionGetBit(vid,8);
    macFunctionSetBit(&tmpHash,11,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,2)^macFunctionGetBit(addrLow,15)^
            macFunctionGetBit(addrLow,28)^macFunctionGetBit(addrHigh,9)^
            macFunctionGetBit(vid,3)^macFunctionGetBit(vid,9);
    macFunctionSetBit(&tmpHash,10,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,3)^macFunctionGetBit(addrLow,16)^
            macFunctionGetBit(addrLow,29)^macFunctionGetBit(addrHigh,10)^
            macFunctionGetBit(vid,4)^macFunctionGetBit(vid,10);
    macFunctionSetBit(&tmpHash,9,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,4)^macFunctionGetBit(addrLow,17)^
            macFunctionGetBit(addrLow,30)^macFunctionGetBit(addrHigh,11)^
            macFunctionGetBit(vid,5)^macFunctionGetBit(vid,11);
    macFunctionSetBit(&tmpHash,8,tmpVal);

    tmpVal = macFunctionGetBit(addrLow,5)^macFunctionGetBit(addrLow,18)^
            macFunctionGetBit(addrLow,31)^macFunctionGetBit(addrHigh,12)^
            macFunctionGetBit(vid,6);
    macFunctionSetBit(&tmpHash,7,tmpVal);

    tmpVal = !(!((macFunctionGetBit(addrLow,6)^macFunctionGetBit(addrLow,19))^
                 macFunctionGetBit(addrHigh,0))^macFunctionGetBit(vid,7));
    macFunctionSetBit(&tmpHash,6,tmpVal);

    tmpVal = !(!((macFunctionGetBit(addrLow,7)^macFunctionGetBit(addrLow,20))^
                 macFunctionGetBit(addrHigh,1))^macFunctionGetBit(vid,8));
    macFunctionSetBit(&tmpHash,5,tmpVal);

    tmpVal = !(!((macFunctionGetBit(addrLow,8)^macFunctionGetBit(addrLow,21))^
                 macFunctionGetBit(addrHigh,2))^macFunctionGetBit(vid,9));
    macFunctionSetBit(&tmpHash,4,tmpVal);

    tmpVal = !(!((macFunctionGetBit(addrLow,9)^macFunctionGetBit(addrLow,22))^
                 macFunctionGetBit(addrHigh,3))^macFunctionGetBit(vid,10));
    macFunctionSetBit(&tmpHash,3,tmpVal);

    tmpVal = !(!((macFunctionGetBit(addrLow,10)^macFunctionGetBit(addrLow,23))^
                 macFunctionGetBit(addrHigh,4))^macFunctionGetBit(vid,11));
    macFunctionSetBit(&tmpHash,2,tmpVal);

    tmpVal = !((macFunctionGetBit(addrLow,11)^macFunctionGetBit(addrLow,24))^
                 macFunctionGetBit(addrHigh,5));
    macFunctionSetBit(&tmpHash,1,tmpVal);

    tmpVal = !((macFunctionGetBit(addrLow,12)^macFunctionGetBit(addrLow,25))^
                 macFunctionGetBit(addrHigh,6))^macFunctionGetBit(vid,0);
    macFunctionSetBit(&tmpHash,0,tmpVal);

    *hash = tmpHash;
    return GT_OK;
}

/*******************************************************************************
* gtMacHashCalcPreperations
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address and VLAN id.
*
* INPUTS:
*       macHashStruct   - the mac hash function configuration.
*       addr            - the mac address.
*       vid             - the VLAN id.
*
* OUTPUTS:
*       low_addr        - the mac address lower bytes
*       high_addr       - the mac address higher bytes.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS gtMacHashCalcPreperations
(
    IN  SGT_MAC_HASH  *   macHashStructPtr,
    IN  GT_ETHERADDR    *addr,
    OUT GT_U32          *low_addr,
    OUT GT_U32          *high_addr
)
{
    GT_U32 AddrHigh;              /* the high address */
    GT_U32 AddrLow;               /* the low address */
    GT_U32 low_addr_;             /* low addrerss after shift */
    GT_U32 high_addr_;            /* high addrerss after shift */
    GT_U32 shift;                 /* the MAC lookup shift */
    AddrHigh = 0;
    AddrHigh = 0;
    low_addr_ = 0;
    high_addr_ = 0;

    if(addr == NULL)
    {
        return GT_BAD_PARAM;
    }

    AddrHigh = addr->arEther[1] | (addr->arEther[0] << 8) ;
    AddrLow = addr->arEther[5] | (addr->arEther[4] << 8) |
        (addr->arEther[3] << 16) | (addr->arEther[2] << 24);

    shift = macHashStructPtr->macShift;

    switch(shift)
    {
        case 1:
            low_addr_ = macFunctionGetBits(AddrLow, 0, 24) << 8 |
                        macFunctionGetBits(AddrHigh, 8, 8);
            high_addr_ = macFunctionGetBits(AddrHigh, 0, 8) << 8 |
                        macFunctionGetBits(AddrLow,24,8);
            break;
        case 2:
            low_addr_ = macFunctionGetBits(AddrHigh, 0, 16) |
                       (macFunctionGetBits(AddrLow, 0, 16) << 16) ;
            high_addr_ =  macFunctionGetBits(AddrLow, 16, 16) ;
            break;
        case 3:
            low_addr_ =  macFunctionGetBits(AddrLow, 0, 8) << 24 |
                         macFunctionGetBits(AddrHigh, 0, 16) << 8 |
                         macFunctionGetBits(AddrLow, 24, 8) ;
            high_addr_ = macFunctionGetBits(AddrLow, 8, 16);
            break;
        case 4:
            low_addr_ = macFunctionGetBits(AddrLow, 16, 16) |
                       (macFunctionGetBits(AddrHigh, 0, 16) << 16);
            high_addr_ = macFunctionGetBits(AddrLow, 0, 16);
            break;
        case 5:
            low_addr_ =  macFunctionGetBits(AddrHigh, 0, 8) << 24 |
                         macFunctionGetBits(AddrLow, 8, 24);
            high_addr_ = macFunctionGetBits(AddrLow, 0, 8) << 8 |
                         macFunctionGetBits(AddrHigh, 8, 8);
            break;
        default:
            low_addr_ = AddrLow;
            high_addr_ = AddrHigh;
            break;
      }
      *high_addr = high_addr_ & (macHashStructPtr->macMask.arEther[0] |
                                (macHashStructPtr->macMask.arEther[1] << 8));
      *low_addr = low_addr_ & (macHashStructPtr->macMask.arEther[2] |
                              (macHashStructPtr->macMask.arEther[3] << 8) |
                              (macHashStructPtr->macMask.arEther[4] << 16) |
                              (macHashStructPtr->macMask.arEther[5] << 24));


      return GT_OK ;
}

/*******************************************************************************
* sgtMacHashCalc
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address and VLAN id.
*
* INPUTS:
*       addr        - the mac address.
*       vid         - the VLAN id.
*       macHashStructPtr - mac hash parameters
* OUTPUTS:
*       hash - the hash index.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS sgtMacHashCalc
(
    IN  GT_ETHERADDR    *   addr,
    IN  GT_U16              vid,
    IN  SGT_MAC_HASH    *   macHashStructPtr,
    OUT GT_U32          *   hash
)
{
      GT_U32 Vid_for_Lookup ;       /* find the vid for lookup */
      GT_U32 low_addr;             /* low_addr_ after mask */
      GT_U32 high_addr;            /* high_addr_ after mask */
      GT_U32 tmpHash;            /* contains the hash in table size 16k */
      GT_STATUS   status;

      tmpHash = 0;

      if((addr == NULL) || (hash == NULL))
      {
          return GT_BAD_PARAM;
      }

      if (macHashStructPtr->vlanMode == GT_IVL)
      {
          switch(macHashStructPtr->vidShift)
          {
            case 1:
                Vid_for_Lookup = (( macFunctionGetBits ( vid , 0 , 8 ) << 4) |
                                  macFunctionGetBits( vid , 8 , 4 )) ;
                break;
            case 2:
                Vid_for_Lookup = (( macFunctionGetBits ( vid , 0 , 4 ) << 8) |
                                  macFunctionGetBits( vid , 4 , 8 )) ;
                break;
            default:
                Vid_for_Lookup = vid ;
          }

          Vid_for_Lookup = Vid_for_Lookup &  macHashStructPtr->vidMask;
      }
      else
      {
          Vid_for_Lookup = 0 ;
      }
      status = gtMacHashCalcPreperations(macHashStructPtr,addr,
                                         &low_addr,&high_addr);
      if (status != GT_OK)
      {
          return status;
      }

      switch (macHashStructPtr->macHashKind)
      {
          case GT_OLD_MAC_HASH_FUNCTION:
            switch (macHashStructPtr->addressMode)
            {
              case GT_MAC_SQN_VLAN_SQN:
                status = gtMacHashMode0(low_addr,high_addr,Vid_for_Lookup,
                                        &tmpHash);
                if (status != GT_OK)
                {
                    return status;
                }
                break;
              case GT_MAC_RND_VLAN_SQN:
                  status = gtMacHashMode1(low_addr,high_addr,Vid_for_Lookup,
                                           &tmpHash);
                  if (status != GT_OK)
                  {
                      return status;
                  }
                  break ;
              default:
                return GT_NOT_SUPPORTED;
            }
            break;
          case GT_NEW_MAC_HASH_FUNCTION:
              status = gtMacHashMode1Fixed(low_addr,high_addr,Vid_for_Lookup,
                                           &tmpHash);
              if (status != GT_OK)
              {
                  return status;
              }
              break ;
          default:
              return GT_NOT_SUPPORTED;
      }


      /* get multiple hash */
      switch(macHashStructPtr->size)
      {
        case SGT_MAC_TBL_4K:  /* 4k - clear bits 11 & 12 & 13 */
            macFunctionSetBit (&tmpHash , 11 , 0 );
            break;
        case SGT_MAC_TBL_8K:  /* 8k - clear bits 12 & 13 */
            macFunctionSetBit (&tmpHash , 12 , 0 );
            break;
        case SGT_MAC_TBL_16K:  /* 16k - clear bits 13 */
            macFunctionSetBit (&tmpHash , 13 , 0 );
            break;
        default:
            return GT_NOT_SUPPORTED;
      }

      tmpHash = tmpHash << 1;
      *hash = tmpHash;
      return GT_OK ;
}

