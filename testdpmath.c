/*******************************************************************
** t e s t d p m a t h . c
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

#include "ficl.h"
#include "dpmath.h"
#include "unity.h"

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

    TEST_MESSAGE("***** Testing cell bit width *****");
    {
        FICL_UNS expected = (FICL_UNS)1 << (sizeof(FICL_UNS) * CHAR_BIT - 1);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE((uint64_t)expected,
                                         (uint64_t)CELL_HI_BIT,
                                         "CELL_HI_BIT");
    }

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

