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
#include "dpmath.h"
#if FICL_UNIT_TEST
    #include "unity.h"
#endif

static  void dpmUModTest();

#if PORTABLE_LONGMULDIV   /* see sysdep.h */
#if !defined(LONGMMULDIV_MAG)   /* Michael Gauland vs the new LLM version */
#define LONGMMULDIV_MAG 0
#endif
#if LONGMULDIV_MAG != 0
    #include "llm.h"
static  DPUNS   dpmAdd(DPUNS x, DPUNS y);
static  DPUNS   dpmASL( DPUNS x );
static  DPUNS   dpmASR( DPUNS x );
static  int     dpmCompare(DPUNS x, DPUNS y);
static  DPUNS   dpmOr( DPUNS x, DPUNS y );
static  DPUNS   dpmSub(DPUNS x, DPUNS y);
#endif /* LONGMULDIV_MAG != 0 */    
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


#if PORTABLE_LONGMULDIV
#if LONGMULDIV_MAG != 0
/**************************************************************************
** Contributed by
** Michael A. Gauland   gaulandm@mdhost.cse.tek.com  
**************************************************************************/

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

#else
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
#endif /* LLM version */

#endif /* PORTABLE_LONGMULDIV */

#if FICL_UNIT_TEST
    static void dpmUModTestCase(DPUNS input, UNS16 base, DPUNS qExpected, UNS16 rExpected)
    {
        UNS16 r   = dpmUMod(&input, base);
    
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(rExpected, r, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(qExpected.hi, input.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(qExpected.lo, input.lo, "Quotient Lo");
    }

    static void ficlLongMulTestCase(FICL_UNS x, FICL_UNS y, DPUNS expected)
    {
        DPUNS got = ficlLongMul(x, y);

        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expected.hi, got.hi, "Mul Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expected.lo, got.lo, "Mul Lo");
    }

    static void ficlLongDivTestCase(DPUNS dividend, FICL_UNS divisor, UNSQR expected)
    {
        UNSQR got = ficlLongDiv(dividend, divisor);

        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expected.quot, got.quot, "Div Quot");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expected.rem, got.rem, "Div Rem");
        TEST_ASSERT_TRUE_MESSAGE(got.rem < divisor, "Div Rem < divisor");

        /* Verify dividend == quot * divisor + rem */
        {
            DPUNS prod = ficlLongMul(got.quot, divisor);
            DPUNS sum = prod;
            sum.lo += got.rem;
            if (sum.lo < prod.lo)
                sum.hi += 1;

            TEST_ASSERT_EQUAL_UINT64_MESSAGE(dividend.hi, sum.hi, "Div Recompose Hi");
            TEST_ASSERT_EQUAL_UINT64_MESSAGE(dividend.lo, sum.lo, "Div Recompose Lo");
        }
    }
    

    void dpmUnitTest(void)
    {
        #define dpmUModTestCase(q, b, eq, er) quot=q; base=b; expQuot=eq; expRem=er; rem=dpmUMod(&quot, base);

        UNS16 rem;
        UNS16 expRem;
        UNS16 base;
        DPUNS quot;
        DPUNS expQuot;
        
        TEST_MESSAGE("***** Testing dpmUMod *****");
        /*--- 1) zero divided by anything → 0, rem 0 ---*/
        dpmUModTestCase(
                ((DPUNS){ .hi = 0, .lo = 0 }),
                1,
                ((DPUNS){ .hi = 0, .lo = 0 }),
                0);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    
        /*--- 2) small < base → quotient 0, rem = value ---*/
        dpmUModTestCase(
                 ((DPUNS){ .hi = 0, .lo = 10 }),
                 20,
                 ((DPUNS){ .hi = 0, .lo = 0 }),
                 10);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    
        /*--- 3) exact divide in lo only ---*/
        dpmUModTestCase(
                 ((DPUNS){ .hi = 0, .lo = 100 }),
                 10,
                 ((DPUNS){ .hi = 0, .lo = 10 }),
                 0);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    
        /*--- 4) power‐of‐two boundary → carry from hi → hi=0, lo=256 ---*/
        dpmUModTestCase(
                 ((DPUNS){ .hi = 0, .lo = 0x10000UL }),
                 0x100,
                 ((DPUNS){ .hi = 0, .lo = 0x100UL }),
                 0);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    
        /*--- 5) single-word value splitting across hi & lo:
              word-size dependent expected quotient ---*/
        {
            DPUNS exp5;
            if (sizeof(FICL_UNS) == 4)
                exp5 = (DPUNS){ .hi = 0, .lo = 0x55555555UL };
            else
                exp5 = (DPUNS){ .hi = 0, .lo = (FICL_UNS)0x5555555555555555ULL };
            dpmUModTestCase(
                     ((DPUNS){ .hi = 1, .lo = 1 }),
                     3,
                     exp5,
                     2);
        }
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
        /* testmain.c:388:dpmUnitTest:FAIL: Expected 1431655765 Was 6148914691236517205. Quotient Lo */

    
        /*--- 6) hi only (value = 2^WORD_BITS) ÷2 → Q=2^(WORD_BITS-1), R=0 ---*/
        {
            DPUNS exp6;
            if (sizeof(FICL_UNS) == 4)
                exp6 = (DPUNS){ .hi = 0, .lo = (FICL_UNS)1 << 31 };
            else
                exp6 = (DPUNS){ .hi = 0, .lo = (FICL_UNS)1 << 63 };
            dpmUModTestCase(
                     ((DPUNS){ .hi = 1, .lo = 0 }),
                     2,
                     exp6,
                     0);
        }
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    
        /*--- 7) max-lo, small base (exact divide) ---*/
        dpmUModTestCase(
                 ((DPUNS){ .hi = 0, .lo = 0xFFFFUL }),
                 0xFF,
                 ((DPUNS){ .hi = 0, .lo = 0x0101UL }),
                 0);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    
        /*--- 8) large hi+lo randomish values ---*/
        {
            DPUNS exp8;
            UNS16 exp8Rem;
            if (sizeof(FICL_UNS) == 4)
            {
                /* Precomputed with a big-int tool or a quick script. */
                exp8 = (DPUNS){ .hi = 0x00111A2EUL, .lo = 0xC80D7BE1UL };
                exp8Rem = 0x09B0;
            }
            else
            {
                exp8 = (DPUNS){
                    .hi = (FICL_UNS)0x0000000000010004ULL,
                    .lo = (FICL_UNS)0xC00E1042CD45CF0BULL
                };
                exp8Rem = 0x0AB4;
            }
            dpmUModTestCase(
                     ((DPUNS){ .hi = 0x12345678UL, .lo = 0x9ABCDEF0UL }),
                     0x1234,
                     exp8,
                     exp8Rem);
        }
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");

        TEST_MESSAGE("***** Testing ficlLongMul *****");
        {
            const FICL_UNS max = (FICL_UNS)~(FICL_UNS)0;
            const int wordBits = (int)(sizeof(FICL_UNS) * CHAR_BIT);
            const FICL_UNS halfBit = (FICL_UNS)1 << (wordBits / 2);
            const FICL_UNS hiBit = (FICL_UNS)1 << (wordBits - 1);

            ficlLongMulTestCase(0, 0, (DPUNS){ .hi = 0, .lo = 0 });
            ficlLongMulTestCase(0, (FICL_UNS)0x12345678UL, (DPUNS){ .hi = 0, .lo = 0 });
            ficlLongMulTestCase(1, (FICL_UNS)0x12345678UL, (DPUNS){ .hi = 0, .lo = (FICL_UNS)0x12345678UL });
            ficlLongMulTestCase(max, 1, (DPUNS){ .hi = 0, .lo = max });

            ficlLongMulTestCase(hiBit, 2, (DPUNS){ .hi = 1, .lo = 0 });
            ficlLongMulTestCase(max, 2, (DPUNS){ .hi = 1, .lo = max - 1 });
            ficlLongMulTestCase(halfBit, halfBit, (DPUNS){ .hi = 1, .lo = 0 });
            ficlLongMulTestCase(max, max, (DPUNS){ .hi = max - 1, .lo = 1 });
            ficlLongMulTestCase(max - 1, max - 1, (DPUNS){ .hi = max - 3, .lo = 4 });

            if (sizeof(FICL_UNS) == 4)
            {
                ficlLongMulTestCase(
                    (FICL_UNS)0x12345678UL,
                    (FICL_UNS)0x9ABCDEF0UL,
                    (DPUNS){ .hi = 0x0B00EA4EUL, .lo = 0x242D2080UL });
                ficlLongMulTestCase(
                    (FICL_UNS)0x00FF00FFUL,
                    (FICL_UNS)0x0F0F0F0FUL,
                    (DPUNS){ .hi = 0x000F000EUL, .lo = 0xFFF0FFF1UL });
            }
            else
            {
                ficlLongMulTestCase(
                    (FICL_UNS)0x12345678UL,
                    (FICL_UNS)0x9ABCDEF0UL,
                    (DPUNS){ .hi = 0, .lo = (FICL_UNS)0x0B00EA4E242D2080ULL });
                ficlLongMulTestCase(
                    (FICL_UNS)0x00FF00FFUL,
                    (FICL_UNS)0x0F0F0F0FUL,
                    (DPUNS){ .hi = 0, .lo = (FICL_UNS)0x000F000EFFF0FFF1ULL });
            }

            {
                const FICL_UNS a = (FICL_UNS)0x1ABCDEUL;
                const FICL_UNS b = (FICL_UNS)0x0FEDCBA9UL;
                DPUNS ab = ficlLongMul(a, b);
                DPUNS ba = ficlLongMul(b, a);
                TEST_ASSERT_EQUAL_UINT64_MESSAGE(ab.hi, ba.hi, "Mul commutative hi");
                TEST_ASSERT_EQUAL_UINT64_MESSAGE(ab.lo, ba.lo, "Mul commutative lo");
            }
        }

        TEST_MESSAGE("***** Testing ficlLongDiv *****");
        {
            const FICL_UNS max = (FICL_UNS)~(FICL_UNS)0;
            const int wordBits = (int)(sizeof(FICL_UNS) * CHAR_BIT);
            const FICL_UNS hiBit = (FICL_UNS)1 << (wordBits - 1);

            ficlLongDivTestCase((DPUNS){ .hi = 0, .lo = 0 }, 1, (UNSQR){ .quot = 0, .rem = 0 });
            ficlLongDivTestCase((DPUNS){ .hi = 0, .lo = 0 }, (FICL_UNS)0x1234UL, (UNSQR){ .quot = 0, .rem = 0 });
            ficlLongDivTestCase((DPUNS){ .hi = 0, .lo = 0x12345678UL }, 1, (UNSQR){ .quot = (FICL_UNS)0x12345678UL, .rem = 0 });
            ficlLongDivTestCase((DPUNS){ .hi = 0, .lo = 0x12345678UL }, (FICL_UNS)0x12345678UL, (UNSQR){ .quot = 1, .rem = 0 });
            ficlLongDivTestCase((DPUNS){ .hi = 0, .lo = 0x12345678UL }, (FICL_UNS)0x12345679UL, (UNSQR){ .quot = 0, .rem = (FICL_UNS)0x12345678UL });

            ficlLongDivTestCase((DPUNS){ .hi = 1, .lo = 0 }, 2, (UNSQR){ .quot = hiBit, .rem = 0 });
            ficlLongDivTestCase((DPUNS){ .hi = 0, .lo = max }, 2, (UNSQR){ .quot = max / 2, .rem = 1 });
            ficlLongDivTestCase((DPUNS){ .hi = 1, .lo = 0 }, max, (UNSQR){ .quot = 1, .rem = 1 });

            ficlLongDivTestCase((DPUNS){ .hi = 0, .lo = 0xFFFFUL }, (FICL_UNS)0xFFUL, (UNSQR){ .quot = 0x0101UL, .rem = 0 });
            ficlLongDivTestCase((DPUNS){ .hi = 0, .lo = 0xFFFFUL }, (FICL_UNS)0x100UL, (UNSQR){ .quot = 0x00FFUL, .rem = 0xFFUL });

            if (sizeof(FICL_UNS) == 4)
            {
                ficlLongDivTestCase(
                    (DPUNS){ .hi = 0x12345678UL, .lo = 0x9ABCDEF0UL },
                    (FICL_UNS)0xFFFFFFFFUL,
                    (UNSQR){ .quot = 0x12345678UL, .rem = 0xACF13568UL });
                ficlLongDivTestCase(
                    (DPUNS){ .hi = 0xCAFEBABEUL, .lo = 0xDEADBEEFUL },
                    (FICL_UNS)0xFFFF0001UL,
                    (UNSQR){ .quot = 0xCAFF85BDUL, .rem = 0x996B3932UL });
                ficlLongDivTestCase(
                    (DPUNS){ .hi = 1, .lo = 1 },
                    3,
                    (UNSQR){ .quot = 0x55555555UL, .rem = 2 });
            }
            else
            {
                ficlLongDivTestCase(
                    (DPUNS){ .hi = 0x12345678UL, .lo = 0x9ABCDEF0UL },
                    (FICL_UNS)0xFFFFFFFFUL,
                    (UNSQR){ .quot = (FICL_UNS)0x1234567812345678ULL, .rem = 0xACF13568UL });
                ficlLongDivTestCase(
                    (DPUNS){ .hi = 0xCAFEBABEUL, .lo = 0xDEADBEEFUL },
                    (FICL_UNS)0xFFFF0001UL,
                    (UNSQR){ .quot = (FICL_UNS)0xCAFF85BCBABD3501ULL, .rem = 0x58F189EEUL });
                ficlLongDivTestCase(
                    (DPUNS){ .hi = 1, .lo = 1 },
                    3,
                    (UNSQR){ .quot = (FICL_UNS)0x5555555555555555ULL, .rem = 2 });
            }
        }
    }
#endif /* FICL_UNIT_TEST */

