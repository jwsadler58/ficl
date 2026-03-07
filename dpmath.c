/*******************************************************************
** d p m a t h . c
** Forth Inspired Command Language - double precision math
** Portable versions of ficlLongMul anb ficlLongDiv
** Author: John W Sadler
** PORTABLE_LONGMULDIV contributed by Michael A. Gauland
** Created: 25 January 1998
** Rev 2.03: 128 bit DP math.
** Rev 2.04: bugfix in dpmAdd - int carry failed with 64 bit operands
**           Renamed prefixes from m64 to dpm (double precision math)
** $Id: math64.c,v 1.6 2001-05-16 07:56:16-07 jsadler Exp jsadler $
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
** Copyright (c) 1997-2026 John W Sadler
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

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include "ficl.h"
#include "dpmath.h"

/**************************************************************************
                        d p m A b s
** Returns the absolute value of an DPINT
**************************************************************************/
DPINT dpmAbs(DPINT x)
{
    if (dpmIsNegative(x))
        x = dpmNegate(x);

    return x;
}


/**************************************************************************
                        d p m F l o o r e d D i v I
**
** FROM THE FORTH ANS...
** Floored division is integer division in which the remainder carries
** the sign of the divisor or is zero, and the quotient is rounded to
** its arithmetic floor. Symmetric division is integer division in which
** the remainder carries the sign of the dividend or is zero and the
** quotient is the mathematical quotient rounded towards zero or
** truncated. Examples of each are shown in tables 3.3 and 3.4.
**
** Table 3.3 - Floored Division Example
** Dividend        Divisor Remainder       Quotient
** --------        ------- ---------       --------
**  10                7       3                1
** -10                7       4               -2
**  10               -7      -4               -2
** -10               -7      -3                1
**
**
** Table 3.4 - Symmetric Division Example
** Dividend        Divisor Remainder       Quotient
** --------        ------- ---------       --------
**  10                7       3                1
** -10                7      -3               -1
**  10               -7       3               -1
** -10               -7      -3                1
**************************************************************************/
INTQR dpmFlooredDivI(DPINT num, FICL_INT den)
{
    INTQR qr;
    UNSQR uqr;
    int signRem = 1;
    int signQuot = 1;

    if (dpmIsNegative(num))
    {
        num = dpmNegate(num);
        signQuot = -signQuot;
    }

    if (den < 0)
    {
        den      = -den;
        signRem  = -signRem;
        signQuot = -signQuot;
    }

    uqr = ficlLongDiv(dpmCastIU(num), (FICL_UNS)den);
    qr = dpmCastQRUI(uqr);
    if (signQuot < 0)
    {
        qr.quot = -qr.quot;
        if (qr.rem != 0)
        {
            qr.quot--;
            qr.rem = den - qr.rem;
        }
    }

    if (signRem < 0)
        qr.rem = -qr.rem;

    return qr;
}


/**************************************************************************
                        d p m I s N e g a t i v e
** Returns TRUE if the specified DPINT has its sign bit set.
**************************************************************************/
int dpmIsNegative(DPINT x)
{
    return x.hi < 0;
}


/**************************************************************************
                        d p m M a c
** Mixed precision multiply and accumulate primitive for number building.
** Multiplies DPUNS u by FICL_UNS mul and adds FICL_UNS add. Mul is typically
** the numeric base, and add represents a digit to be appended to the
** growing number.
** Returns the result of the operation
**************************************************************************/
DPUNS dpmMac(DPUNS u, FICL_UNS mul, FICL_UNS add)
{
    DPUNS resultLo = ficlLongMul(u.lo, mul);
    DPUNS resultHi = ficlLongMul(u.hi, mul);
    resultLo.hi += resultHi.lo;
    resultHi.lo = resultLo.lo + add;

    if (resultHi.lo < resultLo.lo)
        resultLo.hi++;

    resultLo.lo = resultHi.lo;

    return resultLo;
}


/**************************************************************************
                        d p m M u l I
** Multiplies a pair of FICL_INTs and returns an DPINT result.
**************************************************************************/
DPINT dpmMulI(FICL_INT x, FICL_INT y)
{
    DPUNS prod;
    int sign = 1;

    if (x < 0)
    {
        sign = -sign;
        x = -x;
    }

    if (y < 0)
    {
        sign = -sign;
        y = -y;
    }

    prod = ficlLongMul(x, y);
    if (sign > 0)
        return dpmCastUI(prod);
    else
        return dpmNegate(dpmCastUI(prod));
}


/**************************************************************************
                        d p m N e g a t e
** Negates an DPINT by complementing and incrementing.
**************************************************************************/
DPINT dpmNegate(DPINT x)
{
    x.hi = ~x.hi;
    x.lo = ~x.lo;
    x.lo ++;
    if (x.lo == 0)
        x.hi++;

    return x;
}


/**************************************************************************
                        d p m P u s h
** Push an DPINT onto the specified stack in the order required
** by ANS Forth (most significant cell on top)
** These should probably be macros...
**************************************************************************/
void  dpmPushI(FICL_STACK *pStack, DPINT idp)
{
    stackPushINT(pStack, idp.lo);
    stackPushINT(pStack, idp.hi);
    return;
}

void  dpmPushU(FICL_STACK *pStack, DPUNS udp)
{
    stackPushUNS(pStack, udp.lo);
    stackPushUNS(pStack, udp.hi);
    return;
}


/**************************************************************************
                        d p m P o p
** Pops an DPINT off the stack in the order required by ANS Forth
** (most significant cell on top)
** These should probably be macros...
**************************************************************************/
DPINT dpmPopI(FICL_STACK *pStack)
{
    DPINT ret;
    ret.hi = stackPopINT(pStack);
    ret.lo = stackPopINT(pStack);
    return ret;
}

DPUNS dpmPopU(FICL_STACK *pStack)
{
    DPUNS ret;
    ret.hi = stackPopUNS(pStack);
    ret.lo = stackPopUNS(pStack);
    return ret;
}


/**************************************************************************
                        d p m S y m m e t r i c D i v
** Divide a DPINT by a FICL_INT and return a FICL_INT quotient and a
** FICL_INT remainder. The absolute values of quotient and remainder are not
** affected by the signs of the numerator and denominator (the operation
** is symmetric on the number line)
**************************************************************************/
INTQR dpmSymmetricDivI(DPINT num, FICL_INT den)
{
    INTQR qr;
    UNSQR uqr;
    int signRem = 1;
    int signQuot = 1;

    if (dpmIsNegative(num))
    {
        num = dpmNegate(num);
        signRem  = -signRem;
        signQuot = -signQuot;
    }

    if (den < 0)
    {
        den      = -den;
        signQuot = -signQuot;
    }

    uqr = ficlLongDiv(dpmCastIU(num), (FICL_UNS)den);
    qr = dpmCastQRUI(uqr);
    if (signRem < 0)
        qr.rem = -qr.rem;

    if (signQuot < 0)
        qr.quot = -qr.quot;

    return qr;
}


/**************************************************************************
                        d p m U M o d
** Divides a DPUNS by base (an UNS16) and returns an UNS16 remainder.
** Writes the quotient back to the original DPUNS as a side effect.
** This operation is typically used to convert an DPUNS to a text string
** in any base. See words.c:numberSignS, for example.
** Mechanics: performs 4 ficlLongDivs, each of which produces one quarter
** of the quotient. C does not provide a way to divide a FICL_UNS by an
** UNS16 and get a FICL_UNS quotient (ldiv is closest, but it's signed,
** unfortunately), so I've used ficlLongDiv.
** #todo: unit test this for correctness ( # and #s)
**************************************************************************/
UNS16 dpmUMod(DPUNS *pUD, UNS16 base)
{
    assert(base != 0);

    /* bit‐width of one FICL_UNS */
    enum { WORD_BITS = sizeof(FICL_UNS) * CHAR_BIT };
    /* must be even so we can split exactly in half */
    _Static_assert(WORD_BITS % 2 == 0,
                   "FICL_UNS size in bits must be even");
    /* half‐width in bits */
    enum { HALF_BITS = WORD_BITS / 2 };
    /* must be at least 16 so divisor fits */
    _Static_assert(HALF_BITS >= sizeof(UNS16) * CHAR_BIT,
                   "HALF_BITS must be at least 16 bits");

    /* mask for extracting each half‐word */
    const FICL_UNS halfMask = ((FICL_UNS)1 << HALF_BITS) - 1;
    enum { TOTAL = 4 };   /* two halves in hi + two in lo */

    /* break into four “digits” */
    FICL_UNS parts[TOTAL] = {
        (pUD->hi >> HALF_BITS) & halfMask,
         pUD->hi              & halfMask,
        (pUD->lo >> HALF_BITS) & halfMask,
         pUD->lo              & halfMask
    };

    FICL_UNS quot[TOTAL];
    UNS16    rem = 0;

    for (int i = 0; i < TOTAL; i++) {
        /* base-B = 2^HALF_BITS long division step */
        FICL_UNS t = ((FICL_UNS)rem << HALF_BITS) | parts[i];
        quot[i]   = t / base;
        rem        = (UNS16)(t % base);
    }

    /* re-assemble quotient into hi/lo */
    pUD->hi = (quot[0] << HALF_BITS) | quot[1];
    pUD->lo = (quot[2] << HALF_BITS) | quot[3];

    return rem;
}


#if PORTABLE_LONGMULDIV
/*
** LLM versions - no helper funcs needed
*/
DPUNS ficlLongMul(FICL_UNS x, FICL_UNS y)
{
    DPUNS   result;
    size_t  N = sizeof(FICL_UNS) * CHAR_BIT; /* total bits in one word */
    size_t  H = N / 2;                       /* split-point */

    /* mask for the low H bits */
    FICL_UNS mask = ~( (~(FICL_UNS)0) << H );

    /* split each operand into high and low halves */
    FICL_UNS x0 = x & mask;      /* low H bits of x */
    FICL_UNS x1 = x >> H;        /* high H bits of x */
    FICL_UNS y0 = y & mask;      /* low H bits of y */
    FICL_UNS y1 = y >> H;        /* high H bits of y */

    /* four partial products, each at most 2H bits */
    FICL_UNS p00 = x0 * y0;      /* low × low */
    FICL_UNS p01 = x0 * y1;      /* low × high */
    FICL_UNS p10 = x1 * y0;      /* high × low */
    FICL_UNS p11 = x1 * y1;      /* high × high */

    /* break each p__ into its own low and high H-bit halves */
    FICL_UNS p00_lo = p00 & mask;
    FICL_UNS p00_hi = p00 >> H;

    FICL_UNS p01_lo = p01 & mask;
    FICL_UNS p01_hi = p01 >> H;

    FICL_UNS p10_lo = p10 & mask;
    FICL_UNS p10_hi = p10 >> H;

    /* accumulate the “middle” sum and its carry */
    FICL_UNS mid = p00_hi + p01_lo + p10_lo;
    FICL_UNS carry_mid = mid >> H;
    FICL_UNS mid_lo    = mid & mask;

    /* low word =  [mid_lo << H]  |  p00_lo */
    result.lo = (mid_lo << H) | p00_lo;

    /* high word = p11 + p01_hi + p10_hi + carry_mid */
    result.hi = p11 + p01_hi + p10_hi + carry_mid;

    return result;
}

UNSQR ficlLongDiv(DPUNS q, FICL_UNS y)
{
    UNSQR result;
    const size_t bits  = sizeof(FICL_UNS) * CHAR_BIT; /* word size in bits */
    const int    total = (int)(bits * 2);             /* total bits in DPUNS */

    DPUNS   rem  = { 0, 0 };  /* needs bits+1, so keep as 2-word */
    FICL_UNS quot = 0;

    /* optional: guard against divide by zero if not handled elsewhere */
    /* if (y == 0) { result.quot = 0; result.rem = 0; return result; } */

    for (int i = total - 1; i >= 0; --i)
    {
        /* next dividend bit */
        FICL_UNS bit;
        if (i >= (int)bits)
            bit = (q.hi >> (i - (int)bits)) & (FICL_UNS)1;
        else
            bit = (q.lo >> i) & (FICL_UNS)1;

        /* rem = (rem << 1) | bit  (as 2-word shift) */
        {
            FICL_UNS carry = rem.lo >> (bits - 1);
            rem.lo = (rem.lo << 1) | bit;
            rem.hi = (rem.hi << 1) | carry;
        }

        /* if (rem >= y) { rem -= y; set quotient bit } */
        if (rem.hi != 0 || rem.lo >= y)
        {
            /* rem -= y (y is 1-word) */
            FICL_UNS oldlo = rem.lo;
            rem.lo -= y;
            rem.hi -= (oldlo < y) ? 1u : 0u;

            if (i < (int)bits)
                quot |= ((FICL_UNS)1 << i);
        }
    }

    result.quot = quot;
    result.rem  = rem.lo; /* remainder fits in one word */
    return result;
}

#endif /* PORTABLE_LONGMULDIV */
