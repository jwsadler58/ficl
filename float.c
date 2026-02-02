/*******************************************************************
** f l o a t . c
** Forth Inspired Command Language
** ANS Forth FLOAT word-set written in C
** Authors: Guy Carver & John Sadler
** Created: Apr 2001
** $Id: float.c,v 1.8 2001-12-04 17:58:16-08 jsadler Exp jsadler $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include "ficl.h"

#if FICL_WANT_FLOAT

/*******************************************************************
** Do float addition r1 + r2.
** f+ ( r1 r2 -- r )
*******************************************************************/
static void Fadd(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    f = POPFLOAT();
    f += GETTOPF();
    SETTOPF(f);
}

/*******************************************************************
** Do float subtraction r1 - r2.
** f- ( r1 r2 -- r )
*******************************************************************/
static void Fsub(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    f = POPFLOAT();
    f = GETTOPF() - f;
    SETTOPF(f);
}

/*******************************************************************
** Do float multiplication r1 * r2.
** f* ( r1 r2 -- r )
*******************************************************************/
static void Fmul(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    f = POPFLOAT();
    f *= GETTOPF();
    SETTOPF(f);
}

/*******************************************************************
** Do float negation.
** fnegate ( r -- r )
*******************************************************************/
static void Fnegate(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = -GETTOPF();
    SETTOPF(f);
}

/*******************************************************************
** Do float division r1 / r2.
** f/ ( r1 r2 -- r )
*******************************************************************/
static void Fdiv(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    f = POPFLOAT();
    f = GETTOPF() / f;
    SETTOPF(f);
}

/*******************************************************************
** Do float + integer r + n.
** f+i ( r n -- r )
*******************************************************************/
static void Faddi(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
    vmCheckStack(pVM, 1, 0);
#endif

    f = (FICL_FLOAT)POPINT();
    f += GETTOPF();
    SETTOPF(f);
}

/*******************************************************************
** Do float - integer r - n.
** f-i ( r n -- r )
*******************************************************************/
static void Fsubi(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
    vmCheckStack(pVM, 1, 0);
#endif

    f = GETTOPF();
    f -= (FICL_FLOAT)POPINT();
    SETTOPF(f);
}

/*******************************************************************
** Do float * integer r * n.
** f*i ( r n -- r )
*******************************************************************/
static void Fmuli(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
    vmCheckStack(pVM, 1, 0);
#endif

    f = (FICL_FLOAT)POPINT();
    f *= GETTOPF();
    SETTOPF(f);
}

/*******************************************************************
** Do float / integer r / n.
** f/i ( r n -- r )
*******************************************************************/
static void Fdivi(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
    vmCheckStack(pVM, 1, 0);
#endif

    f = GETTOPF();
    f /= (FICL_FLOAT)POPINT();
    SETTOPF(f);
}

/*******************************************************************
** Do integer - float n - r.
** i-f ( n r -- r )
*******************************************************************/
static void isubf(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
    vmCheckStack(pVM, 1, 0);
#endif

    f = (FICL_FLOAT)POPINT();
    f -= GETTOPF();
    SETTOPF(f);
}

/*******************************************************************
** Do integer / float n / r.
** i/f ( n r -- r )
*******************************************************************/
static void idivf(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1,1);
    vmCheckStack(pVM, 1, 0);
#endif

    f = (FICL_FLOAT)POPINT();
    f /= GETTOPF();
    SETTOPF(f);
}

/*******************************************************************
** Do integer to float conversion.
** s>f ( n -- r )
*******************************************************************/
static void sToF(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 1, 0);
    vmCheckFStack(pVM, 0, 1);
#endif

    f = (FICL_FLOAT)POPINT();
    PUSHFLOAT(f);
}

/*******************************************************************
** Do float to integer conversion.
** f>s ( r -- n )
*******************************************************************/
static void fToS(FICL_VM *pVM)
{
    FICL_INT i;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 0, 1);
    vmCheckFStack(pVM, 1, 0);
#endif

    i = (FICL_INT)POPFLOAT();
    PUSHINT(i);
}

/*******************************************************************
** Floating point constant execution word.
*******************************************************************/
void fConstantParen(FICL_VM *pVM)
{
    FICL_WORD *pFW = pVM->runningWord;
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 0, 1);
#endif

    memcpy(&f, pFW->param, sizeof(f));
    PUSHFLOAT(f);
}

/*******************************************************************
** Create a floating point constant.
** fConstant ( r -"name"- )
*******************************************************************/
static void fConstant(FICL_VM *pVM)
{
    FICL_DICT *dp = vmGetDict(pVM);
    STRINGINFO si = vmGetWord(pVM);
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
#endif

    f = POPFLOAT();
    dictAppendWord2(dp, si, fConstantParen, FW_DEFAULT);
    dictAppendFloat(dp, f);
}

#if 0 /* superseded by fDitWithPrecision */
/*******************************************************************
** Display a float in decimal format.
** f. ( r -- )
*******************************************************************/
static void FDot(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
#endif

    f = POPFLOAT();
    sprintf(pVM->pad,"%#f ", (double)f);
    vmTextOut(pVM, pVM->pad, 0);
}
#endif

/*******************************************************************
** Display a float in engineering format.
** fe. ( r -- )
*******************************************************************/
static void EDot(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
#endif

    f = POPFLOAT();
    sprintf(pVM->pad,"%#e ", (double)f);
    vmTextOut(pVM, pVM->pad, 0);
}

/**************************************************************************
                        d i s p l a y F S t a c k
** Display the parameter stack (code for "f.s")
** f.s ( -- )
**************************************************************************/
static void displayFStack(FICL_VM *pVM)
{
    int d = stackDepthFloat(pVM->fStack);
    int i;
    FICL_FLOAT *pFloat;

    vmCheckFStack(pVM, 0, 0);

    vmTextOut(pVM, "F:", 0);

    if (d == 0)
        vmTextOut(pVM, "(Float Stack Empty)", 1);
    else
    {
        ficlLtoa(d, &pVM->pad[1], pVM->base);
        pVM->pad[0] = '[';
        strcat(pVM->pad,"] ");
        vmTextOut(pVM,pVM->pad,0);

        pFloat = pVM->fStack->sp - d;
        for (i = 0; i < d; i++)
        {
            sprintf(pVM->pad, "%.5e ", (double)(*pFloat++));
            vmTextOut(pVM,pVM->pad, 1);
        }
    }
}

/*******************************************************************
** Return size in address units of n floats.
** floats ( n -- n*sizeof(FICL_FLOAT) )
*******************************************************************/
static void Ffloats(FICL_VM *pVM)
{
    FICL_INT n;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 1, 1);
#endif

    n = POPINT();
    PUSHINT(n * (FICL_INT)sizeof(FICL_FLOAT));
}

/*******************************************************************
** Increment address by size of one float.
** float+ ( addr1 -- addr2 )
*******************************************************************/
static void FfloatPlus(FICL_VM *pVM)
{
    char *addr;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 1, 1);
#endif

    addr = (char *)POPPTR();
    addr += sizeof(FICL_FLOAT);
    PUSHPTR(addr);
}

/*******************************************************************
** Runtime for float variables.
*******************************************************************/
static void FvariableParen(FICL_VM *pVM)
{
    FICL_WORD *pFW;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 0, 1);
#endif

    pFW = pVM->runningWord;
    PUSHPTR(pFW->param);
}

/*******************************************************************
** Align dictionary pointer for float storage.
** falign ( -- )
*******************************************************************/
static void Falign(FICL_VM *pVM)
{
    FICL_DICT *dp = vmGetDict(pVM);
    dictAlignFloat(dp);
}

/*******************************************************************
** Return float-aligned address.
** faligned ( addr -- a-addr )
*******************************************************************/
static void Faligned(FICL_VM *pVM)
{
    uintptr_t addr;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 1, 1);
#endif

    addr = (uintptr_t)POPPTR();
    addr = (addr + FICL_FLOAT_ALIGN_MASK) & ~(uintptr_t)FICL_FLOAT_ALIGN_MASK;
    PUSHPTR((void *)addr);
}

/*******************************************************************
** Create a float variable.
** fvariable ( "name" -- )
*******************************************************************/
static void Fvariable(FICL_VM *pVM)
{
    FICL_DICT *dp = vmGetDict(pVM);
    STRINGINFO si = vmGetWord(pVM);
    FICL_WORD *pFW;

    pFW = dictAppendWord2(dp, si, FvariableParen, FW_DEFAULT);
    dictAllotCells(dp, FICL_FLOAT_CELLS);
    memset(pFW->param, 0, sizeof(FICL_FLOAT));
}

/*******************************************************************
** Display a float in scientific format.
** fs. ( r -- )
*******************************************************************/
static void FSdot(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
#endif

    f = POPFLOAT();
    sprintf(pVM->pad, "%.*e ", (int)pVM->fPrecision, (double)f);
    vmTextOut(pVM, pVM->pad, 0);
}


/*******************************************************************
** Trigonometric functions
*******************************************************************/

/*******************************************************************
** fsin ( r1 -- r2 )
*******************************************************************/
static void Fsin(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(sin(f));
}

/*******************************************************************
** fcos ( r1 -- r2 )
*******************************************************************/
static void Fcos(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(cos(f));
}

/*******************************************************************
** ftan ( r1 -- r2 )
*******************************************************************/
static void Ftan(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(tan(f));
}

/*******************************************************************
** fasin ( r1 -- r2 )
*******************************************************************/
static void Fasin(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(asin(f));
}

/*******************************************************************
** facos ( r1 -- r2 )
*******************************************************************/
static void Facos(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(acos(f));
}

/*******************************************************************
** fatan ( r1 -- r2 )
*******************************************************************/
static void Fatan(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(atan(f));
}

/*******************************************************************
** fatan2 ( r1 r2 -- r3 )
*******************************************************************/
static void Fatan2(FICL_VM *pVM)
{
    FICL_FLOAT y, x;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    x = POPFLOAT();
    y = POPFLOAT();
    PUSHFLOAT(atan2(y, x));
}

/*******************************************************************
** fsinh ( r1 -- r2 )
*******************************************************************/
static void Fsinh(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(sinh(f));
}

/*******************************************************************
** fcosh ( r1 -- r2 )
*******************************************************************/
static void Fcosh(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(cosh(f));
}

/*******************************************************************
** ftanh ( r1 -- r2 )
*******************************************************************/
static void Ftanh(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(tanh(f));
}

/*******************************************************************
** Exponential and logarithmic functions
*******************************************************************/

/*******************************************************************
** fexp ( r1 -- r2 )
*******************************************************************/
static void Fexp(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(exp(f));
}

/*******************************************************************
** fln ( r1 -- r2 )
*******************************************************************/
static void Fln(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(log(f));
}

/*******************************************************************
** flog ( r1 -- r2 )
*******************************************************************/
static void Flog(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(log10(f));
}

/*******************************************************************
** flog2 ( r1 -- r2 )
*******************************************************************/
static void Flog2(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(log2(f));
}

/*******************************************************************
** fexp2 ( r1 -- r2 )
*******************************************************************/
static void Fexp2(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(exp2(f));
}

/*******************************************************************
** fexpm1 ( r1 -- r2 )
*******************************************************************/
static void Fexpm1(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(expm1(f));
}

/*******************************************************************
** fln1p ( r1 -- r2 )
*******************************************************************/
static void Fln1p(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(log1p(f));
}

/*******************************************************************
** Power and root functions
*******************************************************************/

/*******************************************************************
** fsqrt ( r1 -- r2 )
*******************************************************************/
static void Fsqrt(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(sqrt(f));
}

/*******************************************************************
** fcbrt ( r1 -- r2 )
** Cube Root
*******************************************************************/
static void Fcbrt(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(cbrt(f));
}

/*******************************************************************
** fpow ( r1 r2 -- r3 )
*******************************************************************/
static void Fpow(FICL_VM *pVM)
{
    FICL_FLOAT base, exponent;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    exponent = POPFLOAT();
    base = POPFLOAT();
    PUSHFLOAT(pow(base, exponent));
}

/*******************************************************************
** f** ( r1 r2 -- r3 )
*******************************************************************/
static void Fpower(FICL_VM *pVM)
{
    Fpow(pVM);
}

/*******************************************************************
** fhypot ( r1 r2 -- r3 )
*******************************************************************/
static void Fhypot(FICL_VM *pVM)
{
    FICL_FLOAT x, y;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    y = POPFLOAT();
    x = POPFLOAT();
    PUSHFLOAT(hypot(x, y));
}

/*******************************************************************
** Rounding and remainder functions
*******************************************************************/

/*******************************************************************
** fabs ( r1 -- r2 )
*******************************************************************/
static void Fabs(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(fabs(f));
}

/*******************************************************************
** ffloor ( r1 -- r2 )
*******************************************************************/
static void Ffloor(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(floor(f));
}

/*******************************************************************
** fceil ( r1 -- r2 )
*******************************************************************/
static void Fceil(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(ceil(f));
}

/*******************************************************************
** fround ( r1 -- r2 )
*******************************************************************/
static void Fround(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(round(f));
}

/*******************************************************************
** ftrunc ( r1 -- r2 )
*******************************************************************/
static void Ftrunc(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(trunc(f));
}

/*******************************************************************
** fmod ( r1 r2 -- r3 )
*******************************************************************/
static void Fmod(FICL_VM *pVM)
{
    FICL_FLOAT x, y;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    y = POPFLOAT();
    x = POPFLOAT();
    PUSHFLOAT(fmod(x, y));
}

/*******************************************************************
** fremainder ( r1 r2 -- r3 )
*******************************************************************/
static void Fremainder(FICL_VM *pVM)
{
    FICL_FLOAT x, y;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    y = POPFLOAT();
    x = POPFLOAT();
    PUSHFLOAT(remainder(x, y));
}

/*******************************************************************
** Utility functions
*******************************************************************/

/*******************************************************************
** fmin ( r1 r2 -- r3 )
*******************************************************************/
static void Fmin(FICL_VM *pVM)
{
    FICL_FLOAT x, y;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    y = POPFLOAT();
    x = POPFLOAT();
    PUSHFLOAT(fmin(x, y));
}

/*******************************************************************
** fmax ( r1 r2 -- r3 )
*******************************************************************/
static void Fmax(FICL_VM *pVM)
{
    FICL_FLOAT x, y;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 1);
#endif

    y = POPFLOAT();
    x = POPFLOAT();
    PUSHFLOAT(fmax(x, y));
}

/*******************************************************************
** fpi ( -- r )
*******************************************************************/
static void Fpi(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 0, 1);
#endif

    PUSHFLOAT((FICL_FLOAT)3.141592653589793238462643383279502884);
}

/*******************************************************************
** fe ( -- r )
*******************************************************************/
static void Fe(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 0, 1);
#endif

    PUSHFLOAT((FICL_FLOAT)2.718281828459045235360287471352662498);
}

/*******************************************************************
** facosh ( r1 -- r2 )
*******************************************************************/
static void Facosh(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(acosh(f));
}

/*******************************************************************
** fasinh ( r1 -- r2 )
*******************************************************************/
static void Fasinh(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(asinh(f));
}

/*******************************************************************
** fatanh ( r1 -- r2 )
*******************************************************************/
static void Fatanh(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(atanh(f));
}

/*******************************************************************
** falog ( r1 -- r2 )
*******************************************************************/
static void Falog(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 1);
#endif

    f = POPFLOAT();
    PUSHFLOAT(pow(10.0, f));
}

/*******************************************************************
** fsincos ( r -- r1 r2 )
*******************************************************************/
static void Fsincos(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 2);
#endif

    f = POPFLOAT();
    PUSHFLOAT(sin(f));
    PUSHFLOAT(cos(f));
}

/*******************************************************************
** precision ( -- u )
** State variable is in pVM->fPrecision
*******************************************************************/
static void Fprecision(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckStack(pVM, 0, 1);
#endif

    PUSHINT(pVM->fPrecision);
}

/*******************************************************************
** set-precision ( u -- )
*******************************************************************/
static void FsetPrecision(FICL_VM *pVM)
{
    int prec;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 1, 0);
#endif

    prec = POPINT();
    if (prec < 1)
        prec = 1;
    if (prec > 17)
        prec = 17;
    pVM->fPrecision = prec;
}

/*******************************************************************
** f. ( r -- )
*******************************************************************/
static void FDotWithPrecision(FICL_VM *pVM)
{
    FICL_FLOAT f;
    char format[32];

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
#endif

    f = POPFLOAT();
    sprintf(format, "%%.%dg ", (int)pVM->fPrecision);
    sprintf(pVM->pad, format, (double)f);
    vmTextOut(pVM, pVM->pad, 0);
}


/*******************************************************************
** Do float stack depth.
** fdepth ( -- n )
*******************************************************************/
static void Fdepth(FICL_VM *pVM)
{
    int i;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 0, 1);
#endif

    i = stackDepthFloat(pVM->fStack);
    PUSHINT(i);
}

/*******************************************************************
** Do float stack drop.
** fdrop ( r -- )
*******************************************************************/
static void Fdrop(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
#endif

    DROPF(1);
}

/*******************************************************************
** Do float stack 2drop.
** f2drop ( r r -- )
*******************************************************************/
static void FtwoDrop(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 0);
#endif

    DROPF(2);
}

/*******************************************************************
** Do float stack dup.
** fdup ( r -- r r )
*******************************************************************/
static void Fdup(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 2);
#endif

    PICKF(0);
}

/*******************************************************************
** Do float stack 2dup.
** f2dup ( r1 r2 -- r1 r2 r1 r2 )
*******************************************************************/
static void FtwoDup(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 4);
#endif

    PICKF(1);
    PICKF(1);
}

/*******************************************************************
** Do float stack over.
** fover ( r1 r2 -- r1 r2 r1 )
*******************************************************************/
static void Fover(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 3);
#endif

    PICKF(1);
}

/*******************************************************************
** Do float stack 2over.
** f2over ( r1 r2 r3 -- r1 r2 r3 r1 r2 )
*******************************************************************/
static void FtwoOver(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 4, 6);
#endif

    PICKF(3);
    PICKF(3);
}

/*******************************************************************
** Do float stack pick.
** fpick ( n -- r )
*******************************************************************/
static void Fpick(FICL_VM *pVM)
{
    CELL c = POP();

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, c.i+1, c.i+2);
#endif

    PICKF(c.i);
}

/*******************************************************************
** Do float stack ?dup.
** f?dup ( r -- r )
*******************************************************************/
static void FquestionDup(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 2);
#endif

    f = GETTOPF();
    if (f != 0)
        PICKF(0);
}

/*******************************************************************
** Do float stack roll.
** froll ( n -- )
*******************************************************************/
static void Froll(FICL_VM *pVM)
{
    int i = POP().i;
    i = (i > 0) ? i : 0;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, i+1, i+1);
#endif

    ROLLF(i);
}

/*******************************************************************
** Do float stack -roll.
** f-roll ( n -- )
*******************************************************************/
static void FminusRoll(FICL_VM *pVM)
{
    int i = POP().i;
    i = (i > 0) ? i : 0;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, i+1, i+1);
#endif

    ROLLF(-i);
}

/*******************************************************************
** Do float stack rot.
** frot ( r1 r2 r3  -- r2 r3 r1 )
*******************************************************************/
static void Frot(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 3, 3);
#endif

    ROLLF(2);
}

/*******************************************************************
** Do float stack -rot.
** f-rot ( r1 r2 r3  -- r3 r1 r2 )
*******************************************************************/
static void Fminusrot(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 3, 3);
#endif

    ROLLF(-2);
}

/*******************************************************************
** Do float stack swap.
** fswap ( r1 r2 -- r2 r1 )
*******************************************************************/
static void Fswap(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 2);
#endif

    ROLLF(1);
}

/*******************************************************************
** Do float stack 2swap
** f2swap ( r1 r2 r3 r4  -- r3 r4 r1 r2 )
*******************************************************************/
static void FtwoSwap(FICL_VM *pVM)
{
#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 4, 4);
#endif

    ROLLF(3);
    ROLLF(3);
}

/*******************************************************************
** Get a floating point number from a variable.
** f@ ( n -- r )
*******************************************************************/
static void Ffetch(FICL_VM *pVM)
{
    FICL_FLOAT *pFloat;
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 0, 1);
    vmCheckStack(pVM, 1, 0);
#endif

    pFloat = (FICL_FLOAT *)POPPTR();
    memcpy(&f, pFloat, sizeof(f));
    PUSHFLOAT(f);
}

/*******************************************************************
** Store a floating point number into a variable.
** f! ( r n -- )
*******************************************************************/
static void Fstore(FICL_VM *pVM)
{
    FICL_FLOAT *pFloat;
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
    vmCheckStack(pVM, 1, 0);
#endif

    pFloat = (FICL_FLOAT *)POPPTR();
    f = POPFLOAT();
    memcpy(pFloat, &f, sizeof(f));
}

/*******************************************************************
** Add a floating point number to contents of a variable.
** f+! ( r n -- )
*******************************************************************/
static void FplusStore(FICL_VM *pVM)
{
    FICL_FLOAT *pFloat;
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 1, 0);
    vmCheckFStack(pVM, 1, 0);
#endif

    pFloat = (FICL_FLOAT *)POPPTR();
    memcpy(&f, pFloat, sizeof(f));
    f += POPFLOAT();
    memcpy(pFloat, &f, sizeof(f));
}

/*******************************************************************
** Floating point literal execution word.
*******************************************************************/
static void fliteralParen(FICL_VM *pVM)
{
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 0, 1);
#endif

    memcpy(&f, (CELL *)pVM->ip, sizeof(f));
    PUSHFLOAT(f);
    vmBranchRelative(pVM, FICL_FLOAT_CELLS);
}

/*******************************************************************
** Compile a floating point literal.
*******************************************************************/
static void fliteralIm(FICL_VM *pVM)
{
    FICL_DICT *dp = vmGetDict(pVM);
    FICL_WORD *pfLitParen = ficlLookup(pVM->pSys, "(fliteral)");
    FICL_FLOAT f;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
#endif

    f = POPFLOAT();
    dictAppendCell(dp, LVALUEtoCELL(pfLitParen));
    dictAppendFloat(dp, f);
}

/*******************************************************************
** Do float 0= comparison r = 0.0.
** f0= ( r -- T/F )
*******************************************************************/
static void FzeroEquals(FICL_VM *pVM)
{
    CELL c;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);                   /* Make sure something on float stack. */
    vmCheckStack(pVM, 0, 1);                    /* Make sure room for result. */
#endif

    c.i = FICL_BOOL(POPFLOAT() == 0);
    PUSH(c);
}

/*******************************************************************
** Do float 0< comparison r < 0.0.
** f0< ( r -- T/F )
*******************************************************************/
static void FzeroLess(FICL_VM *pVM)
{
    CELL c;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);                   /* Make sure something on float stack. */
    vmCheckStack(pVM, 0, 1);                    /* Make sure room for result. */
#endif

    c.i = FICL_BOOL(POPFLOAT() < 0);
    PUSH(c);
}

/*******************************************************************
** Do float 0> comparison r > 0.0.
** f0> ( r -- T/F )
*******************************************************************/
static void FzeroGreater(FICL_VM *pVM)
{
    CELL c;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 1, 0);
    vmCheckStack(pVM, 0, 1);
#endif

    c.i = FICL_BOOL(POPFLOAT() > 0);
    PUSH(c);
}

/*******************************************************************
** Float approximate equality comparison based on python math.isclose
** returns result of: abs(a-b) <= max( relTol * max( abs(a), abs(b) ), absTol )
** AbsTol is useful if one value is zero
** f= ( r1 r2 -- T/F )
*******************************************************************/
static void fIsClose(FICL_VM *pVM)
{
    const FICL_FLOAT relTol = 1e-9;
    const FICL_FLOAT absTol = 2*FICL_FLOAT_EPSILON;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 0);
    vmCheckStack(pVM, 0, 1);
#endif

    FICL_FLOAT b = POPFLOAT();
    FICL_FLOAT a = POPFLOAT();
    FICL_FLOAT diff = fabs(a - b);
    FICL_FLOAT maxAbs = fmax(fabs(a), fabs(b));
    FICL_FLOAT maxDiff = fmax(relTol * maxAbs, absTol);
    PUSHINT(FICL_BOOL(diff <= maxDiff));
}

static void FIsEqual(FICL_VM *pVM)
{
    FICL_FLOAT x,y;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 0);
    vmCheckStack(pVM, 0, 1);
#endif

    y = POPFLOAT();
    x = POPFLOAT();
    PUSHINT(FICL_BOOL(x == y));
}
/*******************************************************************
** Do float < comparison r1 < r2.
** f< ( r1 r2 -- T/F )
*******************************************************************/
static void FisLess(FICL_VM *pVM)
{
    FICL_FLOAT x, y;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 0);
    vmCheckStack(pVM, 0, 1);
#endif

    y = POPFLOAT();
    x = POPFLOAT();
    PUSHINT(FICL_BOOL(x < y));
}

/*******************************************************************
** Do float > comparison r1 > r2.
** f> ( r1 r2 -- T/F )
*******************************************************************/
static void FisGreater(FICL_VM *pVM)
{
    FICL_FLOAT x, y;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 2, 0);
    vmCheckStack(pVM, 0, 1);
#endif

    y = POPFLOAT();
    x = POPFLOAT();
    PUSHINT(FICL_BOOL(x > y));
}


/**************************************************************************
                     F l o a t P a r s e S t a t e
** Enum to determine the current segement of a floating point number
** being parsed.
**************************************************************************/
#define NUMISNEG 1
#define EXPISNEG 2

typedef enum _floatParseState
{
    FPS_START,
    FPS_ININT,
    FPS_INMANT,
    FPS_STARTEXP,
    FPS_INEXP
} FloatParseState;

/**************************************************************************
                     f i c l P a r s e F l o a t N u m b e r
** pVM -- Virtual Machine pointer.
** si -- String to parse.
** Returns 1 if successful, 0 if not.
**************************************************************************/
int ficlParseFloatNumber( FICL_VM *pVM, STRINGINFO si )
{
    unsigned char ch, digit;
    char *cp;
    FICL_COUNT count;
    FICL_FLOAT power;
    FICL_FLOAT accum = 0.0;
    FICL_FLOAT mant = 0.1;
    FICL_INT exponent = 0;
    char flag = 0;
    FloatParseState estate = FPS_START;

#if FICL_ROBUST > 1
    vmCheckFStack(pVM, 0, 1);
#endif

    /*
    ** floating point numbers only allowed in base 10
    */
    if (pVM->base != 10)
        return(0);


    cp = SI_PTR(si);
    count = (FICL_COUNT)SI_COUNT(si);

    /* Loop through the string's characters. */
    while ((count--) && ((ch = *cp++) != 0))
    {
        switch (estate)
        {
            /* At start of the number so look for a sign. */
            case FPS_START:
            {
                estate = FPS_ININT;
                if (ch == '-')
                {
                    flag |= NUMISNEG;
                    break;
                }
                if (ch == '+')
                {
                    break;
                }
            } /* Note!  Drop through to FPS_ININT */
            /*
            **Converting integer part of number.
            ** Only allow digits, decimal and 'E'.
            */
            case FPS_ININT:
            {
                if (ch == '.')
                {
                    estate = FPS_INMANT;
                }
                else if ((ch == 'e') || (ch == 'E'))
                {
                    estate = FPS_STARTEXP;
                }
                else
                {
                    digit = (unsigned char)(ch - '0');
                    if (digit > 9)
                        return(0);

                    accum = accum * 10 + digit;

                }
                break;
            }
            /*
            ** Processing the fraction part of number.
            ** Only allow digits and 'E'
            */
            case FPS_INMANT:
            {
                if ((ch == 'e') || (ch == 'E'))
                {
                    estate = FPS_STARTEXP;
                }
                else
                {
                    digit = (unsigned char)(ch - '0');
                    if (digit > 9)
                        return(0);

                    accum += digit * mant;
                    mant *= 0.1f;
                }
                break;
            }
            /* Start processing the exponent part of number. */
            /* Look for sign. */
            case FPS_STARTEXP:
            {
                estate = FPS_INEXP;

                if (ch == '-')
                {
                    flag |= EXPISNEG;
                    break;
                }
                else if (ch == '+')
                {
                    break;
                }
            }       /* Note!  Drop through to FPS_INEXP */
            /*
            ** Processing the exponent part of number.
            ** Only allow digits.
            */
            case FPS_INEXP:
            {
                digit = (unsigned char)(ch - '0');
                if (digit > 9)
                    return(0);

                exponent = exponent * 10 + digit;

                break;
            }
        }
    }

    /* If parser never made it to the exponent this is not a float. */
    if (estate < FPS_STARTEXP)
        return(0);

    /* Set the sign of the number. */
    if (flag & NUMISNEG)
        accum = -accum;

    /* If exponent is not 0 then adjust number by it. */
    if (exponent != 0)
    {
        /* Determine if exponent is negative. */
        if (flag & EXPISNEG)
        {
            exponent = -exponent;
        }
        /* power = 10^x */
        power = (FICL_FLOAT)pow(10.0, exponent);
        accum *= power;
    }

    PUSHFLOAT(accum);
    if (pVM->state == COMPILE)
        fliteralIm(pVM);

    return(1);
}

/*
** > f l o a t ( c-addr u -- t/f ) F:( - r/~ )
**
*/
static void toFloat(FICL_VM *pVM)
{
    STRINGINFO si;
    si.count = POPINT();
    si.cp  = (char *)POPPTR();

    if (ficlParseFloatNumber(pVM, si))
        PUSHINT(1);
    else
        PUSHINT(0);
}


/**************************************************************************
** Add float words to a system's dictionary.
** pSys -- Pointer to the FICL sytem to add float words to.
**************************************************************************/
void ficlCompileFloat(FICL_SYSTEM *pSys)
{
    FICL_DICT *dp = pSys->dp;
    FICL_VM   *pVM = pSys->vmList;
    assert(dp);
    assert(pVM);

/* 12.6.1 Floating-point words */
    dictAppendWord(  dp, ">float",    toFloat,        FW_DEFAULT);
/*                      d>f         unimplemented  */
    dictAppendOpWord(dp, "f!",        Fstore,         FW_DEFAULT, FICL_OP_FSTORE);
    dictAppendOpWord(dp, "f*",        Fmul,           FW_DEFAULT, FICL_OP_FSTAR);
    dictAppendOpWord(dp, "f+",        Fadd,           FW_DEFAULT, FICL_OP_FPLUS);
    dictAppendOpWord(dp, "f-",        Fsub,           FW_DEFAULT, FICL_OP_FMINUS);
    dictAppendOpWord(dp, "f/",        Fdiv,           FW_DEFAULT, FICL_OP_FSLASH);
    dictAppendOpWord(dp, "f0<",       FzeroLess,      FW_DEFAULT, FICL_OP_F0LESS);
    dictAppendOpWord(dp, "f0=",       FzeroEquals,    FW_DEFAULT, FICL_OP_F0EQUALS);
    dictAppendOpWord(dp, "f<",        FisLess,        FW_DEFAULT, FICL_OP_FLESS);
/*                      f>d         unimplemented  */
    dictAppendOpWord(dp, "f@",        Ffetch,         FW_DEFAULT, FICL_OP_FFETCH);
    dictAppendWord(  dp, "falign",    Falign,         FW_DEFAULT);
    dictAppendWord(  dp, "faligned",  Faligned,       FW_DEFAULT);
    dictAppendWord(  dp, "fconstant", fConstant,      FW_DEFAULT);
    dictAppendOpWord(dp, "fdepth",    Fdepth,         FW_DEFAULT, FICL_OP_FDEPTH);
    dictAppendOpWord(dp, "fdrop",     Fdrop,          FW_DEFAULT, FICL_OP_FDROP);
    dictAppendOpWord(dp, "fdup",      Fdup,           FW_DEFAULT, FICL_OP_FDUP);
    dictAppendWord(  dp, "fliteral",  fliteralIm,     FW_IMMEDIATE);
    dictAppendWord(  dp, "float+",    FfloatPlus,     FW_DEFAULT);
    dictAppendWord(  dp, "floats",    Ffloats,        FW_DEFAULT);
    dictAppendWord(  dp, "floor",     Ffloor,         FW_DEFAULT);
    dictAppendOpWord(dp, "fmax",      Fmax,           FW_DEFAULT, FICL_OP_FMAX);
    dictAppendOpWord(dp, "fmin",      Fmin,           FW_DEFAULT, FICL_OP_FMIN);
    dictAppendOpWord(dp, "fnegate",   Fnegate,        FW_DEFAULT, FICL_OP_FNEGATE);
    dictAppendOpWord(dp, "fover",     Fover,          FW_DEFAULT, FICL_OP_FOVER);
    dictAppendOpWord(dp, "frot",      Frot,           FW_DEFAULT, FICL_OP_FROT);
    dictAppendWord(  dp, "fround",    Fround,         FW_DEFAULT);
    dictAppendOpWord(dp, "fswap",     Fswap,          FW_DEFAULT, FICL_OP_FSWAP);
    dictAppendWord(  dp, "fvariable", Fvariable,      FW_DEFAULT);
/*                      represent   unimplemented */

/*  12.6.2 Floating-point extension words */
/*   #todo: unimplemented FLOAT EXTENSION words
                       df*
                       df@
                       dfalign
                       dfaligned
                       dffield:
                       dfloat+
                       dfloats
                       ffield:
                       fvalue
                       f~           see f~=
                       sf!
                       sf@
                       sfalign
                       sfaligned
                       sffield:
                       sfloat+
                       sfloats
*/
    dictAppendWord(  dp, "f**",       Fpower,         FW_DEFAULT);
    dictAppendWord(  dp, "f.",        FDotWithPrecision, FW_DEFAULT);
    dictAppendOpWord(dp, "f>s",       fToS,           FW_DEFAULT, FICL_OP_F_TO_S);
    dictAppendOpWord(dp, "fabs",      Fabs,           FW_DEFAULT, FICL_OP_FABS);
    dictAppendWord(  dp, "facos",     Facos,          FW_DEFAULT);
    dictAppendWord(  dp, "facosh",    Facosh,         FW_DEFAULT);
    dictAppendWord(  dp, "falog",     Falog,          FW_DEFAULT);
    dictAppendWord(  dp, "fasin",     Fasin,          FW_DEFAULT);
    dictAppendWord(  dp, "fasinh",    Fasinh,         FW_DEFAULT);
    dictAppendWord(  dp, "fatan",     Fatan,          FW_DEFAULT);
    dictAppendWord(  dp, "fatan2",    Fatan2,         FW_DEFAULT);
    dictAppendWord(  dp, "fatanh",    Fatanh,         FW_DEFAULT);
    dictAppendWord(  dp, "fcos",      Fcos,           FW_DEFAULT);
    dictAppendWord(  dp, "fcosh",     Fcosh,          FW_DEFAULT);
    dictAppendWord(  dp, "fe.",       EDot,           FW_DEFAULT);
    dictAppendWord(  dp, "fexp",      Fexp,           FW_DEFAULT);
    dictAppendWord(  dp, "fexp2",     Fexp2,          FW_DEFAULT);
    dictAppendWord(  dp, "fexpm1",    Fexpm1,         FW_DEFAULT);
    dictAppendWord(  dp, "fln",       Fln,            FW_DEFAULT);
    dictAppendWord(  dp, "fln1p",     Fln1p,          FW_DEFAULT);
    dictAppendWord(  dp, "flog",      Flog,           FW_DEFAULT);
    dictAppendWord(  dp, "flog2",     Flog2,          FW_DEFAULT);
    dictAppendWord(  dp, "fs.",       FSdot,          FW_DEFAULT);
    dictAppendWord(  dp, "fsin",      Fsin,           FW_DEFAULT);
    dictAppendWord(  dp, "fsincos",   Fsincos,        FW_DEFAULT);
    dictAppendWord(  dp, "fsinh",     Fsinh,          FW_DEFAULT);
    dictAppendWord(  dp, "fsqrt",     Fsqrt,          FW_DEFAULT);
    dictAppendWord(  dp, "ftan",      Ftan,           FW_DEFAULT);
    dictAppendWord(  dp, "ftanh",     Ftanh,          FW_DEFAULT);
    dictAppendWord(  dp, "ftrunc",    Ftrunc,         FW_DEFAULT);
    dictAppendWord(  dp, "precision", Fprecision,     FW_DEFAULT);
    dictAppendOpWord(dp, "s>f",       sToF,           FW_DEFAULT, FICL_OP_S_TO_F);
    dictAppendWord(  dp, "set-precision",FsetPrecision,FW_DEFAULT);

/*  Ficl floating point extras */
    dictAppendWord(  dp, "fcbrt",     Fcbrt,          FW_DEFAULT);
    dictAppendWord(  dp, "fpow",      Fpow,           FW_DEFAULT);
    dictAppendWord(  dp, "fhypot",    Fhypot,         FW_DEFAULT);
    dictAppendWord(  dp, "fceil",     Fceil,          FW_DEFAULT);
    dictAppendWord(  dp, "fmod",      Fmod,           FW_DEFAULT);
    dictAppendWord(  dp, "fremainder", Fremainder,    FW_DEFAULT);
    dictAppendWord(  dp, "fpi",       Fpi,            FW_DEFAULT);
    dictAppendWord(  dp, "fe",        Fe,             FW_DEFAULT);

    dictAppendWord(  dp, "f.s",       displayFStack,  FW_DEFAULT);
    dictAppendOpWord(dp, "f?dup",     FquestionDup,   FW_DEFAULT, FICL_OP_FQUESTION_DUP);
    dictAppendOpWord(dp, "f~=",       fIsClose,       FW_DEFAULT, FICL_OP_FCLOSE);
    dictAppendOpWord(dp, "f=",        FIsEqual,       FW_DEFAULT, FICL_OP_FEQUAL);
    dictAppendOpWord(dp, "f>",        FisGreater,     FW_DEFAULT, FICL_OP_FGREATER);
    dictAppendOpWord(dp, "f0>",       FzeroGreater,   FW_DEFAULT, FICL_OP_F0GREATER);
    dictAppendOpWord(dp, "f2drop",    FtwoDrop,       FW_DEFAULT, FICL_OP_F2DROP);
    dictAppendOpWord(dp, "f2dup",     FtwoDup,        FW_DEFAULT, FICL_OP_F2DUP);
    dictAppendOpWord(dp, "f2over",    FtwoOver,       FW_DEFAULT, FICL_OP_F2OVER);
    dictAppendOpWord(dp, "f2swap",    FtwoSwap,       FW_DEFAULT, FICL_OP_F2SWAP);
    dictAppendOpWord(dp, "f+!",       FplusStore,     FW_DEFAULT, FICL_OP_FPLUS_STORE);
    dictAppendOpWord(dp, "f+i",       Faddi,          FW_DEFAULT, FICL_OP_FPLUS_I);
    dictAppendOpWord(dp, "f-i",       Fsubi,          FW_DEFAULT, FICL_OP_FMINUS_I);
    dictAppendOpWord(dp, "f*i",       Fmuli,          FW_DEFAULT, FICL_OP_FSTAR_I);
    dictAppendOpWord(dp, "f/i",       Fdivi,          FW_DEFAULT, FICL_OP_FSLASH_I);
    dictAppendOpWord(dp, "fpick",     Fpick,          FW_DEFAULT, FICL_OP_FPICK);
    dictAppendOpWord(dp, "froll",     Froll,          FW_DEFAULT, FICL_OP_FROLL);
    dictAppendOpWord(dp, "i-f",       isubf,          FW_DEFAULT, FICL_OP_I_MINUS_F);
    dictAppendOpWord(dp, "i/f",       idivf,          FW_DEFAULT, FICL_OP_I_SLASH_F);
    dictAppendOpWord(dp, "f-roll",    FminusRoll,     FW_DEFAULT, FICL_OP_FMINUS_ROLL);
    dictAppendOpWord(dp, "f-rot",     Fminusrot,      FW_DEFAULT, FICL_OP_FMINUS_ROT);
    dictAppendWord(  dp, "(fliteral)", fliteralParen, FW_COMPILE);

    ficlSetEnv(pSys, "floating",       FICL_TRUE);
    ficlSetEnv(pSys, "floating-ext",   FICL_TRUE);
    ficlSetEnv(pSys, "floating-stack", pVM->fStack->nCells);
    ficlSetEnvF(pSys, "max-float",    FICL_FLT_MAX);
    ficlSetEnvF(pSys, "float-epsilon", FICL_FLOAT_EPSILON);
    return;
}
#endif  /* FICL_WANT_FLOAT */
