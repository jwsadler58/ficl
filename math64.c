/*******************************************************************
** m a t h 6 4 . c
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

#include <assert.h>
#include <limits.h>
#include "ficl.h"
#include "math64.h"
#if FICL_UNIT_TEST
    #include "unity.h"
#endif

static  void dpmUModTest();

#if PORTABLE_LONGMULDIV   /* see sysdep.h */
static  DPUNS   dpmAdd(DPUNS x, DPUNS y);
static  DPUNS   dpmASL( DPUNS x );
static  DPUNS   dpmASR( DPUNS x );
static  int     dpmCompare(DPUNS x, DPUNS y);
static  DPUNS   dpmOr( DPUNS x, DPUNS y );
static  DPUNS   dpmSub(DPUNS x, DPUNS y);
#endif


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
    return (x.hi < 0);
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
#if 0

UNS16 dpmUMod(DPUNS *pUD, UNS16 base)
{
    DPUNS ud;
    UNSQR qr;
    DPUNS result;
    enum { UMOD_SHIFT = CELL_BITS/2};
    enum { UMOD_MASK = ((FICL_UNS)1 << UMOD_SHIFT) - 1 };

    result.hi = result.lo = 0;

    ud.hi = 0;
    ud.lo = pUD->hi >> UMOD_SHIFT;
    qr = ficlLongDiv(ud, (FICL_UNS)base);
    result.hi = qr.quot << UMOD_SHIFT;

    ud.lo = (qr.rem << UMOD_SHIFT) | (pUD->hi & UMOD_MASK);
    qr = ficlLongDiv(ud, (FICL_UNS)base);
    result.hi |= qr.quot & UMOD_MASK;

    ud.lo = (qr.rem << UMOD_SHIFT) | (pUD->lo >> UMOD_SHIFT);
    qr = ficlLongDiv(ud, (FICL_UNS)base);
    result.lo = qr.quot << UMOD_SHIFT;

    ud.lo = (qr.rem << UMOD_SHIFT) | (pUD->lo & UMOD_MASK);
    qr = ficlLongDiv(ud, (FICL_UNS)base);
    result.lo |= qr.quot & UMOD_MASK;

    *pUD = result;

    return (UNS16)(qr.rem);
}

#else

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
    enum { HALF_MASK = ((FICL_UNS)1 << HALF_BITS) - 1 };
    enum { TOTAL = 4 };   /* two halves in hi + two in lo */

    /* break into four “digits” */
    FICL_UNS parts[TOTAL] = {
        (pUD->hi >> HALF_BITS) & HALF_MASK,
         pUD->hi              & HALF_MASK,
        (pUD->lo >> HALF_BITS) & HALF_MASK,
         pUD->lo              & HALF_MASK
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
#endif


/**************************************************************************
** Contributed by
** Michael A. Gauland   gaulandm@mdhost.cse.tek.com  
**************************************************************************/
#if PORTABLE_LONGMULDIV
/**************************************************************************
                        d p m A d d
** Double precision add
** 08 Jul 2025 - revised carry detection for correct behavior 
**************************************************************************/
static DPUNS dpmAdd(DPUNS x, DPUNS y)
{
    DPUNS         result;
    FICL_UNS      oldLo;

    /* add the high halves first */
    result.hi = x.hi + y.hi;

    /* save old low word, then add */
    oldLo     = x.lo;
    result.lo = oldLo + y.lo;

    /* if the low‐word addition overflowed, bump the high half */
    if (result.lo < oldLo)
        result.hi++;

    return result;
}

/**************************************************************************
                        d p m S u b
** Double precision subtract
**************************************************************************/
static DPUNS dpmSub(DPUNS x, DPUNS y)
{
    DPUNS result;
    
    result.hi = x.hi - y.hi;
    result.lo = x.lo - y.lo;

    if (x.lo < y.lo) 
    {
        result.hi--;
    }

    return result;
}


/**************************************************************************
                        d p m A S L
** Double precision left shift
**************************************************************************/
static DPUNS dpmASL( DPUNS x )
{
    DPUNS result;
    
    result.hi = x.hi << 1;
    if (x.lo & CELL_HI_BIT) 
    {
        result.hi++;
    }

    result.lo = x.lo << 1;

    return result;
}


/**************************************************************************
                        d p m A S R
** Double precision unsigned right shift (no sign extend)
**************************************************************************/
static DPUNS dpmASR( DPUNS x )
{
    DPUNS result;
    
    result.lo = x.lo >> 1;
    if (x.hi & 1) 
    {
        result.lo |= CELL_HI_BIT;
    }

    result.hi = x.hi >> 1;
    return result;
}


/**************************************************************************
                        d p m O r
** Double precision bitwise OR
**************************************************************************/
static DPUNS dpmOr( DPUNS x, DPUNS y )
{
    DPUNS result;
    
    result.hi = x.hi | y.hi;
    result.lo = x.lo | y.lo;
    
    return result;
}


/**************************************************************************
                        d p m C o m p a r e
** Double precision comparison
** Return -1 if x < y; 0 if x==y, and 1 if x > y.
**************************************************************************/
static int dpmCompare(DPUNS x, DPUNS y)
{
    int result;
    
    if (x.hi > y.hi) 
    {
        result = +1;
    } 
    else if (x.hi < y.hi) 
    {
        result = -1;
    } 
    else 
    {
        /* High parts are equal */
        if (x.lo > y.lo) 
        {
            result = +1;
        } 
        else if (x.lo < y.lo) 
        {
            result = -1;
        } 
        else 
        {
            result = 0;
        }
    }
    
    return result;
}


/**************************************************************************
                        f i c l L o n g M u l
** Portable versions of ficlLongMul and ficlLongDiv in C
** Contributed by:
** Michael A. Gauland   gaulandm@mdhost.cse.tek.com  
**************************************************************************/
DPUNS ficlLongMul(FICL_UNS x, FICL_UNS y)
{
    DPUNS result = { 0, 0 };
    DPUNS addend;
    
    addend.lo = y;
    addend.hi = 0; /* No sign extension--arguments are unsigned */
    
    while (x != 0) 
    {
        if ( x & 1) 
        {
            result = dpmAdd(result, addend);
        }
        x >>= 1;
        addend = dpmASL(addend);
    }
    return result;
}


/**************************************************************************
                        f i c l L o n g D i v
** Portable versions of ficlLongMul and ficlLongDiv in C
** Contributed by:
** Michael A. Gauland   gaulandm@mdhost.cse.tek.com  
**************************************************************************/
UNSQR ficlLongDiv(DPUNS q, FICL_UNS y)
{
    UNSQR result;
    DPUNS quotient;
    DPUNS subtrahend;
    DPUNS mask;

    quotient.lo = 0;
    quotient.hi = 0;
    
    subtrahend.lo = y;
    subtrahend.hi = 0;
    
    mask.lo = 1;
    mask.hi = 0;
    
    while ((dpmCompare(subtrahend, q) < 0) &&
           (subtrahend.hi & CELL_HI_BIT) == 0)
    {
        mask = dpmASL(mask);
        subtrahend = dpmASL(subtrahend);
    }
    
    while (mask.lo != 0 || mask.hi != 0) 
    {
        if (dpmCompare(subtrahend, q) <= 0) 
        {
            q = dpmSub( q, subtrahend);
            quotient = dpmOr(quotient, mask);
        }
        mask = dpmASR(mask);
        subtrahend = dpmASR(subtrahend);
    }
    
    result.quot = quotient.lo;
    result.rem = q.lo;
    return result;
}

#if 0
/*
** o4-mini-high versions - no helper funcs needed
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
    size_t bits  = sizeof(FICL_UNS) * CHAR_BIT; /* N = word size in bits */
    int    total = (int)(bits * 2);             /* total bits in DPUNS */
    FICL_UNS rem  = 0;
    FICL_UNS quot = 0;

    /* Long division, one bit at a time from MSB down to LSB */
    for (int i = total - 1; i >= 0; --i) {
        /* shift remainder left, bring in next bit of dividend */
        rem <<= 1;
        if (i >= (int)bits) {
            /* bit comes from the high word */
            rem |= (q.hi >> (i - bits)) & (FICL_UNS)1;
        }
        else {
            /* bit comes from the low word */
            rem |= (q.lo >> i) & (FICL_UNS)1;
        }

        /* subtract divisor if we can, and set quotient bit */
        if (rem >= y) {
            rem -= y;
            /* only store quotient bits that fit in one word */
            if (i < (int)bits) {
                quot |= ( (FICL_UNS)1 << i );
            }
        }
    }

    result.quot = quot;
    result.rem  = rem;
    return result;
}
#endif /* o4-mini-high */

#endif /* PORTABLE_LONGMULDIV */

