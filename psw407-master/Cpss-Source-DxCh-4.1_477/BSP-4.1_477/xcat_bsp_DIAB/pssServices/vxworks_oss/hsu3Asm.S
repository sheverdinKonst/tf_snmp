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
        
/*******************************************************************************
* hsu1Asm.S - hsu feature
*
* DESCRIPTION:
*       Hitless Software Upgrade supported by BSP.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/

#define SYNC1  nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop
#define CP_DELAY bl cp_delay
        
/* See also ARM926EJ-S Technical Reference Manual */
#define C1_MMU          (1<<0)          /* mmu off/on */
#define C1_ALIGN        (1<<1)          /* alignment faults off/on */
#define C1_DC           (1<<2)          /* dcache off/on */

#define C1_BIG_ENDIAN   (1<<7)          /* big endian off/on */
#define C1_SYS_PROT     (1<<8)          /* system protection */
#define C1_ROM_PROT     (1<<9)          /* ROM protection */
#define C1_IC           (1<<12)         /* icache off/on */
#define C1_HIGH_VECTORS (1<<13)         /* location of vectors: low/high addresses */

        .align  2
        .type    cp_delay,function
cp_delay:
        mov     r3, #0
        b       2f
1:      add     r3, r3, #1
        
2:      cmp     r3, #99
        ble     1b
        mov     pc, lr
        
                
        .align  2
        .global hsuRebootBootRomAsm
        .type    hsuRebootBootRomAsm,function
hsuRebootBootRomAsm:
/* 
    Lower the cpu back to ground zero: 
     Disable caches.
     Drain caches.
     Turn off mmu.
     Set cpu address decoding windows to default.        
    Jump to bootrom address = 0xffff0000
*/
    
/* disable interrupts */        
        mrs     r2, cpsr
        orr     r3, r2, #0xc0
        msr     cpsr_c, r3
        SYNC1
        
/* cleanup caches */        
        
        /* turn off L2 Cache */
        mrc     p15, 1, r3, c15, c1, 0
        bic     r3, r3, #0x00400000     /* r3 &= ~0x00400000; */
        mcr     p15, 1, r3, c15, c1, 0
        SYNC1
        
        /* Clean L2 Cache */
        mcr     p15, 1, r3, c15, c9, 0
        SYNC1
        
        /* Drain write buffer */
        mcr     p15, 0, r3, c7, c10, 4
        SYNC1

        /* turn off I/D-cache */
        mrc     p15, 0, r3, c1, c0, 0
        SYNC1        
        bic     r3, r3, #C1_IC
        bic     r3, r3, #C1_DC          /* r3 &= ~(C1_DC | C1_IC); */
        mcr     p15, 0, r3, c1, c0, 0
        
        /* Check if we are 926 or 946 */
        mrc p15, 0, r3, c0, c0, 0
        SYNC1        
        mov     r3, r3, lsr #0x4
        mov     r3, r3, asl #20
        mov     r3, r3, lsr #20
        ldr     r2, =0x926
        
        /* if (((r3 >> 4)&0xfff) == 0x926) */
        
        cmp     r3, r2
        bne     2f
        
        /* flush I/D-cache 926*/
        mov     r3, #0
        mcr     p15, 0, r3, c7, c7, 0
        b       3f
2:
        /* flush I/D-cache else*/
        mov     r3, #0
        mcr p15, 0, r3, c7, c5, 0
        mcr p15, 0, r3, c7, c6, 0
3:
        SYNC1

/* disable mmu */
        mrc     p15, 0, r0, c1, c0, 0   @ read control reg
        mov     r4, r0
        CP_DELAY
        bic     r0, r4, #C1_MMU
        mcr     p15, 0, r0, c1, c0, 0   @ write it back
        SYNC1

/* fix cpu adw's (address decoding windows) to default */
        
        ldr r0, =0XF1020000
        ldr r1, =0X1FFFE841
        str r1, [r0, #0]
        
        ldr r0, =0XF1020004
        ldr r1, =0X80000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020008
        ldr r1, =0X80000000
        str r1, [r0, #0]
        
        ldr r0, =0XF102000C
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020010
        ldr r1, =0X07FF2F11
        str r1, [r0, #0]
        
        ldr r0, =0XF1020014
        ldr r1, =0XD8000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020018
        ldr r1, =0XD8000000
        str r1, [r0, #0]
        
        ldr r0, =0XF102001C
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020020
        ldr r1, =0X0000E041
        str r1, [r0, #0]
        
        ldr r0, =0XF1020024
        ldr r1, =0XC0000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020028
        ldr r1, =0XC0000000
        str r1, [r0, #0]
        
        ldr r0, =0XF102002C
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020030
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020034
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020038
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF102003C
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        
        ldr r0, =0XF1020040
        ldr r1, =0X00000131
        str r1, [r0, #0]
        
        ldr r0, =0XF1020044
        ldr r1, =0XC8010000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020048
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF102004C
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020050
        ldr r1, =0X07FF1E11
        str r1, [r0, #0]
        
        ldr r0, =0XF1020054
        ldr r1, =0XE8000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020058
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF102005C
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020060
        ldr r1, =0X07FF1D11
        str r1, [r0, #0]
        
        ldr r0, =0XF1020064
        ldr r1, =0XF0000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020068
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF102006C
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020070
        ldr r1, =0X07FF1D11
        str r1, [r0, #0]
        
        ldr r0, =0XF1020074
        ldr r1, =0XF8000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020078
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF102007C
        ldr r1, =0X00000000
        str r1, [r0, #0]
        
        ldr r0, =0XF1020080
        ldr r1, =0XD0000000
        str r1, [r0, #0]
        
        SYNC1

/*
  More code to support nand boot
  The base reg is now 0xd0000000 
*/


/* play with BOOTROM_ROUTINE_ERR_REG and fake a new powerup to bootrom */
        ldr r0, =0XD00100D0
        ldr r1, =0XFFFF0000
        str r1, [r0, #0]

/* stop the timer completely!! vxWorks leaves a periodical timer running. */
        
        ldr r0, =0XD0020300
        ldr r1, =0X00000000
        str r1, [r0, #0]

        ldr r0, =0XD0020310
        ldr r1, =0X00000000
        str r1, [r0, #0]        
        
/* jump to bootrom address 0xffff0000 */
        ldr     r3, =0xffff0000
        mov     pc, r3
        
