/*******************************************************************
** m a t h 6 4 . h
** Forth Inspired Command Language - double precision math 
** Portable versions of ficlLongMul anb ficlLongDiv
** Author: John W Sadler 
** PORTABLE_LONGMULDIV contributed by Michael A. Gauland
** Created: 25 January 1998
** Rev 2.03: 128 bit DP math. This file ought to be renamed!
** Rev 2.04: renamed prefixes to dpm rather than m64. 
** $Id: math64.h,v 1.6 2001-05-16 07:56:19-07 jsadler Exp jsadler $
*******************************************************************/
/*
** Get the latest Ficl release at https://sourceforge.net/projects/ficl/
**
** I am interested in hearing from anyone who uses ficl. If you have
** a problem, a success story, a bug or bugfix, a suggestion, or
** if you would like to contribute to Ficl, please contact me on sourceforge.
**
** L I C E N S E  and  D I S C L A I M E R
** 
** Copyright (c) 1997-2025 John W Sadler 
** All rights reserved.
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its contributors
**    may be used to endorse or promote products derived from this software 
**    without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#if !defined (__MATH64_H__)
#define __MATH64_H__

#ifdef __cplusplus
extern "C" {
#endif

DPINT   dpmAbs(DPINT x);
int     dpmIsNegative(DPINT x);
DPUNS   dpmMac(DPUNS u, FICL_UNS mul, FICL_UNS add);
DPINT   dpmMulI(FICL_INT x, FICL_INT y);
DPINT   dpmNegate(DPINT x);
INTQR   dpmFlooredDivI(DPINT num, FICL_INT den);
void    dpmPushI(FICL_STACK *pStack, DPINT idp);
DPINT   dpmPopI(FICL_STACK *pStack);
void    dpmPushU(FICL_STACK *pStack, DPUNS udp);
DPUNS   dpmPopU(FICL_STACK *pStack);
INTQR   dpmSymmetricDivI(DPINT num, FICL_INT den);
UNS16   dpmUMod(DPUNS *pUD, UNS16 base);
#if FICL_UNIT_TEST
    void dpmUnitTest(void);
#endif

#define dpmExtendI(idp) (idp).hi = ((idp).lo < 0) ? -1L : 0 
#define dpmCastIU(idp) (*(DPUNS *)(&(idp)))
#define dpmCastUI(udp) (*(DPINT *)(&(udp)))
#define dpmCastQRIU(iqr) (*(UNSQR *)(&(iqr)))
#define dpmCastQRUI(uqr) (*(INTQR *)(&(uqr)))

#define CELL_HI_BIT (1L << (CELL_BITS-1))

#ifdef __cplusplus
}
#endif

#endif /* __MATH64_H__ */

