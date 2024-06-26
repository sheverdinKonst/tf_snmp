/*******************************************************************************
   Copyright (C) Marvell International Ltd. and its affiliates

   This software file (the "File") is owned and distributed by Marvell
   International Ltd. and/or its affiliates ("Marvell") under the following
   alternative licensing terms.  Once you have made an election to distribute the
   File under one of the following license alternatives, please (i) delete this
   introductory statement regarding license alternatives, (ii) delete the two
   license alternatives that you have not elected to use and (iii) preserve the
   Marvell copyright notice above.

********************************************************************************
   Marvell Commercial License Option

   If you received this File from Marvell and you have entered into a commercial
   license agreement (a "Commercial License") with Marvell, the File is licensed
   to you under the terms of the applicable Commercial License.

********************************************************************************
   Marvell GPL License Option

   If you received this File from Marvell, you may opt to use, redistribute and/or
   modify this File in accordance with the terms and conditions of the General
   Public License Version 2, June 1991 (the "GPL License"), a copy of which is
   available along with the File in the license.txt file or by writing to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
   on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

   THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
   WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
   DISCLAIMED.  The GPL License provides additional details about this warranty
   disclaimer.
********************************************************************************
   Marvell BSD License Option

   If you received this File from Marvell, you may opt to use, redistribute and/or
   modify this File under the following licensing terms.
   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

*   Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

*   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

*   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "mvOs.h"
#include "mv802_3.h"
#include "mvCommon.h"
#include "mvTypes.h"

/*******************************************************************************
* mvMacStrToHex - Convert MAC format string to hex.
*
* DESCRIPTION:
*       This function convert MAC format string to hex.
*
* INPUT:
*       macStr - MAC address string. Format of address string is
*                uu:vv:ww:xx:yy:zz, where ":" can be any delimiter.
*
* OUTPUT:
*       macHex - MAC in hex format.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_STATUS mvMacStrToHex(const char *macStr, MV_U8 *macHex)
{
    int i;
    char tmp[3];

    for (i = 0; i < MV_MAC_ADDR_SIZE; i++)
    {
        tmp[0] = macStr[(i * 3) + 0];
        tmp[1] = macStr[(i * 3) + 1];
        tmp[2] = '\0';
        macHex[i] = (MV_U8) (strtol (tmp, NULL, 16));
    }

    return MV_OK;
}

/*******************************************************************************
* mvMacHexToStr - Convert MAC in hex format to string format.
*
* DESCRIPTION:
*		This function convert MAC in hex format to string format.
*
* INPUT:
*       macHex - MAC in hex format.
*
* OUTPUT:
*       macStr - MAC address string. String format is uu:vv:ww:xx:yy:zz.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_STATUS mvMacHexToStr(MV_U8 *macHex, char *macStr)
{
    int i;

    for (i = 0; i < MV_MAC_ADDR_SIZE; i++)
    {
        mvOsSPrintf (&macStr[i * 3], "%02x:", macHex[i]);
    }
    macStr[(i * 3) - 1] = '\0';

    return MV_OK;
}

/*******************************************************************************
* mvSizePrint - Print the given size with size unit description.
*
* DESCRIPTION:
*		This function print the given size with size unit description.
*       FOr example when size paramter is 0x180000, the function prints:
*       "size 1MB+500KB"
*
* INPUT:
*       size - Size in bytes.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvSizePrint(MV_U32 size)
{
    mvOsOutput ("size ");

    if (size >= _1G)
    {
        mvOsOutput ("%3dGB ", size / _1G);
        size %= _1G;
        if (size)
        {
            mvOsOutput ("+");
        }
    }
    if (size >= _1M)
    {
        mvOsOutput ("%3dMB ", size / _1M);
        size %= _1M;
        if (size)
        {
            mvOsOutput ("+");
        }
    }
    if (size >= _1K)
    {
        mvOsOutput ("%3dKB ", size / _1K);
        size %= _1K;
        if (size)
        {
            mvOsOutput ("+");
        }
    }
    if (size > 0)
    {
        mvOsOutput ("%3dB ", size);
    }
}

/*******************************************************************************
* mvHexToBin - Convert hex to binary
*
* DESCRIPTION:
*		This function Convert hex to binary.
*
* INPUT:
*       pHexStr - hex buffer pointer.
*       size    - Size to convert.
*
* OUTPUT:
*       pBin - Binary buffer pointer.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvHexToBin(const char *pHexStr, MV_U8 *pBin, int size)
{
    int     j, i;
    char    tmp[3];
    MV_U8   byte;

    for (j = 0, i = 0; j < size; j++, i += 2)
    {
        tmp[0] = pHexStr[i];
        tmp[1] = pHexStr[i + 1];
        tmp[2] = '\0';
        byte = (MV_U8) (strtol (tmp, NULL, 16) & 0xFF);
        pBin[j] =  byte;
    }
}

void     mvAsciiToHex(const char *asciiStr, char *hexStr)
{
    int i = 0;

    while (asciiStr[i] != 0)
    {
        mvOsSPrintf (&hexStr[i * 2], "%02x", asciiStr[i]);
        i++;
    }
    hexStr[i * 2] = 0;
}

void    mvBinToHex(const MV_U8 *bin, char *hexStr, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        mvOsSPrintf (&hexStr[i * 2], "%02x", bin[i]);
    }
    hexStr[i * 2] = '\0';
}

void    mvBinToAscii(const MV_U8 *bin, char *asciiStr, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        mvOsSPrintf (&asciiStr[i * 2], "%c", bin[i]);
    }
    asciiStr[i * 2] = '\0';
}

/*******************************************************************************
* mvLog2 -
*
* DESCRIPTION:
*	Calculate the Log2 of a given number.
*
* INPUT:
*       num - A number to calculate the Log2 for.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Log 2 of the input number, or 0xFFFFFFFF if input is 0.
*
*******************************************************************************/
MV_U32 mvLog2(MV_U32 num)
{
    MV_U32 result = 0;
    if (num == 0)
    {
        return 0xFFFFFFFF;
    }
    while (num != 1)
    {
        num = num >> 1;
        result++;
    }
    return result;
}

/*******************************************************************************
 * mvPrintGndPktInfo
 */
MV_VOID mvPrintPktBuffer(MV_U8 *buffP, MV_U32 len)
{
    MV_U32 i;

    mvOsPrintf ("\n");

    for (i = 0; i < len; i++)
    {
        mvOsPrintf ("%02X", buffP[i]);
    }

    mvOsPrintf ("\n\n");
}

/*******************************************************************************
 * mvPrintGndPktInfo
 */
MV_VOID mvPrintGndPktInfo(MV_GND_PKT_INFO *pktInfoP)
{
    mvOsPrintf ("pktInfoP           = 0x%08X.\n", (MV_U32)pktInfoP);
    mvOsPrintf ("pktInfoP->osInfo   = 0x%08X.\n", (MV_U32)pktInfoP->osInfo);
    mvOsPrintf ("pktInfoP->pFrags   = 0x%08X.\n", (MV_U32)pktInfoP->pFrags);

    mvPrintGndBufInfo (pktInfoP->pFrags);

    mvOsPrintf ("pktInfoP->status   = 0x%08X.\n", (MV_U32)pktInfoP->status);
    mvOsPrintf ("pktInfoP->pktSize  = 0x%08X.\n", (MV_U32)pktInfoP->pktSize);
    mvOsPrintf ("pktInfoP->numFrags = 0x%08X.\n", (MV_U32)pktInfoP->numFrags);
    mvOsPrintf ("pktInfoP->srcIdx   = 0x%08X.\n", (MV_U32)pktInfoP->srcIdx);
    mvOsPrintf ("pktInfoP->fragIP   = 0x%08X.\n", (MV_U32)pktInfoP->fragIP);
    mvOsPrintf ("pktInfoP->ownerId  = 0x%08X.\n", (MV_U32)pktInfoP->ownerId);

    mvPrintExtDsa (&pktInfoP->extDsa);

    mvOsPrintf ("pktInfoP->nextP    = 0x%08X.\n",(MV_U32) pktInfoP->osInfo);
}

/*******************************************************************************
 * mvPrintGndBufInfo
 */
MV_VOID mvPrintGndBufInfo(MV_GND_BUF_INFO *bufInfoP)
{
    mvOsPrintf ("bufInfoP->bufVirtPtr   = 0x%08X.\n", (MV_U32)bufInfoP->bufVirtPtr);
    mvOsPrintf ("bufInfoP->bufPhysAddr  = 0x%08X.\n", (MV_U32)bufInfoP->bufPhysAddr);
    mvOsPrintf ("bufInfoP->bufSize      = 0x%08X.\n", (MV_U32)bufInfoP->bufSize);
    mvOsPrintf ("bufInfoP->dataSize     = 0x%08X.\n", (MV_U32)bufInfoP->dataSize);
    mvOsPrintf ("bufInfoP->memHandle    = 0x%08X.\n", (MV_U32)bufInfoP->memHandle);

    mvPrintPktBuffer (bufInfoP->bufVirtPtr, bufInfoP->dataSize);
}

/*******************************************************************************
 * mvPrintExtDsa
 */
MV_VOID mvPrintExtDsa(MV_EXT_DSA *extDsaP)
{
    mvOsPrintf ("extDsaP->word0    = 0x%08X.\n", extDsaP->ww.word0);
    mvOsPrintf ("extDsaP->word1    = 0x%08X.\n", extDsaP->ww.word1);
}

/*******************************************************************************
 * mvBuffsToPktInfo
 */
MV_VOID mvBuffsToPktInfo(MV_U8 *          buffs[],
                         MV_U32           buffsLen[],
                         MV_U32           buffNum,
                         MV_GND_PKT_INFO *pktInfoP)
{
    MV_GND_BUF_INFO  *bufInfoP;
    MV_U32            i;

    /*
     * Fill packet wrapper.
     */
    bufInfoP = pktInfoP->pFrags;
    for (i = 0; i < buffNum; i++)
    {
        bufInfoP[i].bufVirtPtr   = buffs[i];
    #ifdef ETH_DESCR_IN_HIGH_MEM
        bufInfoP[i].bufVirtPtr  = (MV_U8 * )bspPhys2Virt ((MV_U32)(buffs[i]));
        bufInfoP[i].bufPhysAddr  = (MV_ULONG)buffs[i];
    #else
        bufInfoP[i].bufPhysAddr  = mvOsIoVirtToPhy (NULL, buffs[i]);
    #endif
        bufInfoP[i].dataSize     = buffsLen[i];

        pktInfoP->pktSize       += buffsLen[i];
    }
    pktInfoP->numFrags = buffNum;
    pktInfoP->status   = 0;
}

/*******************************************************************************
 * mvEthInitBuffInfo
 */
MV_VOID mvEthInitBuffInfo(MV_GND_BUF_INFO  *bufInfoP,
                          MV_U8            *buffP,
                          MV_U32            buffLen)
{
    bufInfoP->bufVirtPtr   = buffP;
#ifdef ETH_DESCR_IN_HIGH_MEM
    bufInfoP->bufVirtPtr  = (MV_U8* )bspPhys2Virt((MV_U32)(buffP));
    bufInfoP->bufPhysAddr  = (MV_ULONG)buffP;
#else
    bufInfoP->bufPhysAddr  = mvOsIoVirtToPhy(NULL, buffP);
#endif
    bufInfoP->dataSize     = buffLen;
}

/*******************************************************************************
* mvWinOverlapTest
*
* DESCRIPTION:
*       This function checks the given two address windows for overlaping.
*
* INPUT:
*       pAddrWin1 - Address window 1.
*       pAddrWin2 - Address window 2.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if address window overlaps, MV_FALSE otherwise.
*
*******************************************************************************/
MV_STATUS mvWinOverlapTest(MV_ADDR_WIN *pAddrWin1, MV_ADDR_WIN *pAddrWin2)
{
    MV_U32 winBase1, winBase2;
    MV_U32 winTop1, winTop2;

    /* check if we have overflow than 4G*/
    if (((0xffffffff - pAddrWin1->baseLow) < pAddrWin1->size-1)||
       ((0xffffffff - pAddrWin2->baseLow) < pAddrWin2->size-1))
    {
        return MV_TRUE;
    }

    winBase1 = pAddrWin1->baseLow;
    winBase2 = pAddrWin2->baseLow;
    winTop1  = winBase1 + pAddrWin1->size-1;
    winTop2  = winBase2 + pAddrWin2->size-1;


    if (((winBase1 <= winTop2 ) && ( winTop2 <= winTop1)) ||
        ((winBase1 <= winBase2) && (winBase2 <= winTop1)))
    {
        return MV_TRUE;
    }
    else
    {
        return MV_FALSE;
    }
}


