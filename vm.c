/*******************************************************************
** v m . c
** Forth Inspired Command Language - virtual machine methods
** Author: John W Sadler
** Created: 19 July 1997
** $Id: vm.c,v 1.12 2001-12-04 17:58:14-08 jsadler Exp jsadler $
*******************************************************************/
/*
** This file implements the virtual machine of FICL. Each virtual
** machine retains the state of an interpreter. A virtual machine
** owns a pair of stacks for parameters and return addresses, as
** well as a pile of state variables and the two dedicated registers
** of the interp.
*/
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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "ficl.h"
#include <float.h>
#include <math.h>
#include "dpmath.h"

#if FICL_ROBUST > 1
    #define VM_CHECK_STACK_LOCAL(pop, push) \
        do { \
            int depth = (int)(dataTop - pVM->pStack->base); \
            if (depth < (pop)) { \
                pVM->pStack->sp = dataTop; \
                vmThrowUnderflow(pVM); \
            } \
            if (depth - (pop) + (push) > (int)pVM->pStack->nCells) { \
                pVM->pStack->sp = dataTop; \
                vmThrowOverflow(pVM); \
            } \
        } while (0)
#else
    #define VM_CHECK_STACK_LOCAL(pop, push) \
        do { } while (0)
#endif

#if FICL_WANT_FLOAT
    #if FICL_ROBUST > 1
        /* Float stack cached - use local floatTop variable */
        #define VM_CHECK_FSTACK_LOCAL(pop, push) \
            do { \
                int fdepth = (int)(floatTop - pVM->fStack->base); \
                if (fdepth < (pop)) { \
                    pVM->fStack->sp = floatTop; \
                    pVM->pStack->sp = dataTop; \
                    vmThrowUnderflow(pVM); \
                } \
                if (fdepth - (pop) + (push) > (int)pVM->fStack->nCells) { \
                    pVM->fStack->sp = floatTop; \
                    pVM->pStack->sp = dataTop; \
                    vmThrowOverflow(pVM); \
                } \
            } while (0)
    #else
        #define VM_CHECK_FSTACK_LOCAL(pop, push) \
            do { } while (0)
    #endif
#endif

/*
** X-Macro definitions for stack operations
** Each operation is defined once and can be instantiated with different continuations.
** The generator takes: (label, name, pop_count, push_count, code_block)
** where 'label' is the goto target (OP_DONE or OP_CONTINUE)
*/
#define GEN_OP_CASE_LABEL(label, name, pop, push, code) \
    case FICL_OP_##name: { \
        VM_CHECK_STACK_LOCAL(pop, push); \
        code \
        goto label; \
    }

/* Old macro now uses X-macro (fully backward compatible) */
#define VM_OP_CASES_BASE(OP_DONE) \
    STACK_OPS_LIST_WITH_LABEL(OP_DONE, GEN_OP_CASE_LABEL)

#define STACK_OPS_LIST_WITH_LABEL(label, OP) \
    OP(label, DUP, 1, 2, { \
        *dataTop = dataTop[-1]; \
        dataTop++; \
    }) \
    OP(label, DROP, 1, 0, { \
        dataTop--; \
    }) \
    OP(label, SWAP, 2, 2, { \
        c = dataTop[-1]; \
        dataTop[-1] = dataTop[-2]; \
        dataTop[-2] = c; \
    }) \
    OP(label, OVER, 2, 3, { \
        *dataTop = dataTop[-2]; \
        dataTop++; \
    }) \
    OP(label, ROT, 3, 3, { \
        c = dataTop[-3]; \
        dataTop[-3] = dataTop[-2]; \
        dataTop[-2] = dataTop[-1]; \
        dataTop[-1] = c; \
    }) \
    OP(label, MINUS_ROT, 3, 3, { \
        c = dataTop[-1]; \
        dataTop[-1] = dataTop[-2]; \
        dataTop[-2] = dataTop[-3]; \
        dataTop[-3] = c; \
    }) \
    OP(label, PICK, 1, 1, { \
        i = dataTop[-1].i; \
        if (i >= 0) { \
            VM_CHECK_STACK_LOCAL(i + 2, i + 2); \
            dataTop[-1] = dataTop[-i - 2]; \
        } \
    }) \
    OP(label, ROLL, 1, 0, { \
        i = (--dataTop)->i; \
        if (i > 0) { \
            VM_CHECK_STACK_LOCAL(i + 1, i + 1); \
            c = dataTop[-i - 1]; \
            memmove(&dataTop[-i - 1], &dataTop[-i], i * sizeof(CELL)); \
            dataTop[-1] = c; \
        } \
    }) \
    OP(label, MINUS_ROLL, 1, 0, { \
        i = (--dataTop)->i; \
        if (i > 0) { \
            VM_CHECK_STACK_LOCAL(i + 1, i + 1); \
            c = dataTop[-1]; \
            memmove(&dataTop[-i], &dataTop[-i - 1], i * sizeof(CELL)); \
            dataTop[-i - 1] = c; \
        } \
    }) \
    OP(label, 2DUP, 2, 4, { \
        dataTop[0] = dataTop[-2]; \
        dataTop[1] = dataTop[-1]; \
        dataTop += 2; \
    }) \
    OP(label, 2DROP, 2, 0, { \
        dataTop -= 2; \
    }) \
    OP(label, 2SWAP, 4, 4, { \
        c = dataTop[-1]; \
        c2 = dataTop[-2]; \
        dataTop[-1] = dataTop[-3]; \
        dataTop[-2] = dataTop[-4]; \
        dataTop[-3] = c; \
        dataTop[-4] = c2; \
    }) \
    OP(label, 2OVER, 4, 6, { \
        dataTop[0] = dataTop[-4]; \
        dataTop[1] = dataTop[-3]; \
        dataTop += 2; \
    }) \
    OP(label, QUESTION_DUP, 1, 2, { \
        if (dataTop[-1].i != 0) { \
            *dataTop = dataTop[-1]; \
            dataTop++; \
        } \
    }) \
    OP(label, FETCH, 1, 1, { \
        dataTop[-1] = *(CELL *)dataTop[-1].p; \
    }) \
    OP(label, STORE, 2, 0, { \
        CELL *addr = (CELL *)(--dataTop)->p; \
        *addr = *--dataTop; \
    }) \
    OP(label, 2FETCH, 1, 2, { \
        CELL *addr = (CELL *)dataTop[-1].p; \
        dataTop[-1] = addr[0]; \
        *dataTop = addr[1]; \
        dataTop++; \
        c = dataTop[-1]; \
        dataTop[-1] = dataTop[-2]; \
        dataTop[-2] = c; \
    }) \
    OP(label, 2STORE, 3, 0, { \
        CELL *addr = (CELL *)(--dataTop)->p; \
        addr[0] = *--dataTop; \
        addr[1] = *--dataTop; \
    }) \
    OP(label, PLUS_STORE, 2, 0, { \
        CELL *addr = (CELL *)(--dataTop)->p; \
        addr->i += (--dataTop)->i; \
    }) \
    OP(label, C_FETCH, 1, 1, { \
        dataTop[-1].u = (FICL_UNS)(*(UNS8 *)dataTop[-1].p); \
    }) \
    OP(label, C_STORE, 2, 0, { \
        UNS8 *addr = (UNS8 *)(--dataTop)->p; \
        *addr = (UNS8)(--dataTop)->u; \
    }) \
    OP(label, W_FETCH, 1, 1, { \
        dataTop[-1].u = (FICL_UNS)(*(UNS16 *)dataTop[-1].p); \
    }) \
    OP(label, W_STORE, 2, 0, { \
        UNS16 *addr = (UNS16 *)(--dataTop)->p; \
        *addr = (UNS16)(--dataTop)->u; \
    }) \
    OP(label, PLUS, 2, 1, { \
        i = (--dataTop)->i; \
        dataTop[-1].i += i; \
    }) \
    OP(label, MINUS, 2, 1, { \
        i = (--dataTop)->i; \
        dataTop[-1].i -= i; \
    }) \
    OP(label, STAR, 2, 1, { \
        i = (--dataTop)->i; \
        dataTop[-1].i *= i; \
    }) \
    OP(label, SLASH, 2, 1, { \
        i = (--dataTop)->i; \
        dataTop[-1].i /= i; \
    }) \
    OP(label, MOD, 2, 1, { \
        DPINT d1; \
        INTQR qr; \
        i = (--dataTop)->i; \
        d1.lo = (--dataTop)->i; \
        dpmExtendI(d1); \
        qr = dpmSymmetricDivI(d1, i); \
        dataTop->i = qr.rem; \
        dataTop++; \
    }) \
    OP(label, SLASH_MOD, 2, 2, { \
        DPINT d1; \
        INTQR qr; \
        i = (--dataTop)->i; \
        d1.lo = (--dataTop)->i; \
        dpmExtendI(d1); \
        qr = dpmSymmetricDivI(d1, i); \
        dataTop[0].i = qr.rem; \
        dataTop[1].i = qr.quot; \
        dataTop += 2; \
    }) \
    OP(label, STAR_SLASH, 3, 1, { \
        DPINT prod; \
        i = (--dataTop)->i; \
        c = *--dataTop; \
        c2 = *--dataTop; \
        prod = dpmMulI(c2.i, c.i); \
        dataTop->i = dpmSymmetricDivI(prod, i).quot; \
        dataTop++; \
    }) \
    OP(label, STAR_SLASH_MOD, 3, 2, { \
        DPINT prod; \
        INTQR qr; \
        i = (--dataTop)->i; \
        c = *--dataTop; \
        c2 = *--dataTop; \
        prod = dpmMulI(c2.i, c.i); \
        qr = dpmSymmetricDivI(prod, i); \
        dataTop[0].i = qr.rem; \
        dataTop[1].i = qr.quot; \
        dataTop += 2; \
    }) \
    OP(label, ONE_PLUS, 1, 1, { \
        dataTop[-1].i += 1; \
    }) \
    OP(label, ONE_MINUS, 1, 1, { \
        dataTop[-1].i -= 1; \
    }) \
    OP(label, TWO_STAR, 1, 1, { \
        dataTop[-1].i *= 2; \
    }) \
    OP(label, TWO_SLASH, 1, 1, { \
        dataTop[-1].i >>= 1; \
    }) \
    OP(label, NEGATE, 1, 1, { \
        dataTop[-1].i = -dataTop[-1].i; \
    }) \
    OP(label, MAX, 2, 1, { \
        i = (--dataTop)->i; \
        if (dataTop[-1].i < i) \
            dataTop[-1].i = i; \
    }) \
    OP(label, MIN, 2, 1, { \
        i = (--dataTop)->i; \
        if (dataTop[-1].i > i) \
            dataTop[-1].i = i; \
    }) \
    OP(label, ZERO_LESS, 1, 1, { \
        i = (--dataTop)->i; \
        dataTop->i = FICL_BOOL(i < 0); \
        dataTop++; \
    }) \
    OP(label, ZERO_EQUALS, 1, 1, { \
        i = (--dataTop)->i; \
        dataTop->i = FICL_BOOL(i == 0); \
        dataTop++; \
    }) \
    OP(label, ZERO_GREATER, 1, 1, { \
        i = (--dataTop)->i; \
        dataTop->i = FICL_BOOL(i > 0); \
        dataTop++; \
    }) \
    OP(label, LESS, 2, 1, { \
        i = (--dataTop)->i; \
        c = *--dataTop; \
        dataTop->i = FICL_BOOL(c.i < i); \
        dataTop++; \
    }) \
    OP(label, EQUALS, 2, 1, { \
        c = *--dataTop; \
        i = (--dataTop)->i; \
        dataTop->i = FICL_BOOL(i == c.i); \
        dataTop++; \
    }) \
    OP(label, GREATER, 2, 1, { \
        i = (--dataTop)->i; \
        c = *--dataTop; \
        dataTop->i = FICL_BOOL(c.i > i); \
        dataTop++; \
    }) \
    OP(label, U_LESS, 2, 1, { \
        u = (--dataTop)->u; \
        c = *--dataTop; \
        dataTop->i = FICL_BOOL(c.u < u); \
        dataTop++; \
    }) \
    OP(label, AND, 2, 1, { \
        c = *--dataTop; \
        dataTop[-1].i &= c.i; \
    }) \
    OP(label, OR, 2, 1, { \
        c = *--dataTop; \
        dataTop[-1].i |= c.i; \
    }) \
    OP(label, XOR, 2, 1, { \
        c = *--dataTop; \
        dataTop[-1].i ^= c.i; \
    }) \
    OP(label, INVERT, 1, 1, { \
        dataTop[-1].i = ~dataTop[-1].i; \
    }) \
    OP(label, LSHIFT, 2, 1, { \
        u = (--dataTop)->u; \
        dataTop[-1].u <<= u; \
    }) \
    OP(label, RSHIFT, 2, 1, { \
        u = (--dataTop)->u; \
        dataTop[-1].u >>= u; \
    }) \
    OP(label, TO_R, 1, 0, { \
        *pVM->rStack->sp++ = *--dataTop; \
    }) \
    OP(label, R_FROM, 0, 1, { \
        *dataTop++ = *--pVM->rStack->sp; \
    }) \
    OP(label, R_FETCH, 0, 1, { \
        *dataTop++ = pVM->rStack->sp[-1]; \
    }) \
    OP(label, 2TO_R, 2, 0, { \
        c = dataTop[-1]; \
        dataTop[-1] = dataTop[-2]; \
        dataTop[-2] = c; \
        *pVM->rStack->sp++ = *--dataTop; \
        *pVM->rStack->sp++ = *--dataTop; \
    }) \
    OP(label, 2R_FROM, 0, 2, { \
        *dataTop++ = *--pVM->rStack->sp; \
        *dataTop++ = *--pVM->rStack->sp; \
        c = dataTop[-1]; \
        dataTop[-1] = dataTop[-2]; \
        dataTop[-2] = c; \
    }) \
    OP(label, 2R_FETCH, 0, 2, { \
        dataTop[0] = pVM->rStack->sp[-2]; \
        dataTop[1] = pVM->rStack->sp[-1]; \
        dataTop += 2; \
    }) \
    OP(label, DEPTH, 0, 1, { \
        dataTop->i = (FICL_INT)(dataTop - pVM->pStack->base); \
        dataTop++; \
    })

/* Old VM_OP_CASES_BASE duplicate code removed - now generated from X-macro above */

#if FICL_WANT_FLOAT
    /* Macro to access float stack top - abstracts whether it's cached or not */
    #define FSTACK_TOP floatTop

    #define VM_OP_CASES_FLOAT(OP_DONE) \
        case FICL_OP_FDUP: { \
            VM_CHECK_FSTACK_LOCAL(1, 2); \
            *FSTACK_TOP = FSTACK_TOP[-1]; \
            FSTACK_TOP++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FDROP: { \
            VM_CHECK_FSTACK_LOCAL(1, 0); \
            FSTACK_TOP--; \
            goto OP_DONE; \
        } \
        case FICL_OP_FSWAP: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(2, 2); \
            f = FSTACK_TOP[-1]; \
            FSTACK_TOP[-1] = FSTACK_TOP[-2]; \
            FSTACK_TOP[-2] = f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FOVER: { \
            VM_CHECK_FSTACK_LOCAL(2, 3); \
            *FSTACK_TOP = FSTACK_TOP[-2]; \
            FSTACK_TOP++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FROT: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(3, 3); \
            f = FSTACK_TOP[-3]; \
            FSTACK_TOP[-3] = FSTACK_TOP[-2]; \
            FSTACK_TOP[-2] = FSTACK_TOP[-1]; \
            FSTACK_TOP[-1] = f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FMINUS_ROT: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(3, 3); \
            f = FSTACK_TOP[-1]; \
            FSTACK_TOP[-1] = FSTACK_TOP[-2]; \
            FSTACK_TOP[-2] = FSTACK_TOP[-3]; \
            FSTACK_TOP[-3] = f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FPICK: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            i = (--dataTop)->i; \
            VM_CHECK_FSTACK_LOCAL(i + 1, i + 2); \
            *FSTACK_TOP = FSTACK_TOP[-i - 1]; \
            FSTACK_TOP++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FROLL: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            i = (--dataTop)->i; \
            if (i < 0) \
                i = 0; \
            VM_CHECK_FSTACK_LOCAL(i + 1, i + 1); \
            if (i > 0) { \
                FICL_FLOAT f = FSTACK_TOP[-i - 1]; \
                memmove(&FSTACK_TOP[-i - 1], &FSTACK_TOP[-i], i * sizeof(FICL_FLOAT)); \
                FSTACK_TOP[-1] = f; \
            } \
            goto OP_DONE; \
        } \
        case FICL_OP_FMINUS_ROLL: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            i = (--dataTop)->i; \
            if (i < 0) \
                i = 0; \
            VM_CHECK_FSTACK_LOCAL(i + 1, i + 1); \
            if (i > 0) { \
                FICL_FLOAT f = FSTACK_TOP[-1]; \
                memmove(&FSTACK_TOP[-i], &FSTACK_TOP[-i - 1], i * sizeof(FICL_FLOAT)); \
                FSTACK_TOP[-i - 1] = f; \
            } \
            goto OP_DONE; \
        } \
        case FICL_OP_F2DUP: { \
            VM_CHECK_FSTACK_LOCAL(2, 4); \
            FSTACK_TOP[0] = FSTACK_TOP[-2]; \
            FSTACK_TOP[1] = FSTACK_TOP[-1]; \
            FSTACK_TOP += 2; \
            goto OP_DONE; \
        } \
        case FICL_OP_F2DROP: { \
            VM_CHECK_FSTACK_LOCAL(2, 0); \
            FSTACK_TOP -= 2; \
            goto OP_DONE; \
        } \
        case FICL_OP_F2SWAP: { \
            FICL_FLOAT f1, f2; \
            VM_CHECK_FSTACK_LOCAL(4, 4); \
            f1 = FSTACK_TOP[-1]; \
            f2 = FSTACK_TOP[-2]; \
            FSTACK_TOP[-1] = FSTACK_TOP[-3]; \
            FSTACK_TOP[-2] = FSTACK_TOP[-4]; \
            FSTACK_TOP[-3] = f1; \
            FSTACK_TOP[-4] = f2; \
            goto OP_DONE; \
        } \
        case FICL_OP_F2OVER: { \
            VM_CHECK_FSTACK_LOCAL(4, 6); \
            FSTACK_TOP[0] = FSTACK_TOP[-4]; \
            FSTACK_TOP[1] = FSTACK_TOP[-3]; \
            FSTACK_TOP += 2; \
            goto OP_DONE; \
        } \
        case FICL_OP_FQUESTION_DUP: { \
            VM_CHECK_FSTACK_LOCAL(1, 2); \
            if (FSTACK_TOP[-1] != 0) { \
                *FSTACK_TOP = FSTACK_TOP[-1]; \
                FSTACK_TOP++; \
            } \
            goto OP_DONE; \
        } \
        case FICL_OP_FPLUS: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(2, 1); \
            f = *--FSTACK_TOP; \
            FSTACK_TOP[-1] += f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FMINUS: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(2, 1); \
            f = *--FSTACK_TOP; \
            FSTACK_TOP[-1] -= f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FSTAR: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(2, 1); \
            f = *--FSTACK_TOP; \
            FSTACK_TOP[-1] *= f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FSLASH: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(2, 1); \
            f = *--FSTACK_TOP; \
            FSTACK_TOP[-1] /= f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FNEGATE: { \
            VM_CHECK_FSTACK_LOCAL(1, 1); \
            FSTACK_TOP[-1] = -FSTACK_TOP[-1]; \
            goto OP_DONE; \
        } \
        case FICL_OP_FABS: { \
            VM_CHECK_FSTACK_LOCAL(1, 1); \
            FSTACK_TOP[-1] = (FICL_FLOAT)fabs((double)FSTACK_TOP[-1]); \
            goto OP_DONE; \
        } \
        case FICL_OP_FMAX: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(2, 1); \
            f = *--FSTACK_TOP; \
            if (FSTACK_TOP[-1] < f) \
                FSTACK_TOP[-1] = f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FMIN: { \
            FICL_FLOAT f; \
            VM_CHECK_FSTACK_LOCAL(2, 1); \
            f = *--FSTACK_TOP; \
            if (FSTACK_TOP[-1] > f) \
                FSTACK_TOP[-1] = f; \
            goto OP_DONE; \
        } \
        case FICL_OP_FPLUS_STORE: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(1, 0); \
            { \
                FICL_FLOAT *addr = (FICL_FLOAT *)(--dataTop)->p; \
                *addr += *--FSTACK_TOP; \
            } \
            goto OP_DONE; \
        } \
        case FICL_OP_FFETCH: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(0, 1); \
            FSTACK_TOP[0] = *(FICL_FLOAT *)(--dataTop)->p; \
            FSTACK_TOP++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FSTORE: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(1, 0); \
            *(FICL_FLOAT *)(--dataTop)->p = *--FSTACK_TOP; \
            goto OP_DONE; \
        } \
        case FICL_OP_F0LESS: { \
            VM_CHECK_STACK_LOCAL(0, 1); \
            VM_CHECK_FSTACK_LOCAL(1, 0); \
            dataTop->i = FICL_BOOL(*--FSTACK_TOP < 0); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_F0EQUALS: { \
            VM_CHECK_STACK_LOCAL(0, 1); \
            VM_CHECK_FSTACK_LOCAL(1, 0); \
            dataTop->i = FICL_BOOL(*--FSTACK_TOP == 0); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_F0GREATER: { \
            VM_CHECK_STACK_LOCAL(0, 1); \
            VM_CHECK_FSTACK_LOCAL(1, 0); \
            dataTop->i = FICL_BOOL(*--FSTACK_TOP > 0); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FLESS: { \
            FICL_FLOAT f; \
            VM_CHECK_STACK_LOCAL(0, 1); \
            VM_CHECK_FSTACK_LOCAL(2, 0); \
            f = *--FSTACK_TOP; \
            dataTop->i = FICL_BOOL(*--FSTACK_TOP < f); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FGREATER: { \
            FICL_FLOAT f; \
            VM_CHECK_STACK_LOCAL(0, 1); \
            VM_CHECK_FSTACK_LOCAL(2, 0); \
            f = *--FSTACK_TOP; \
            dataTop->i = FICL_BOOL(*--FSTACK_TOP > f); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FCLOSE: { \
            FICL_FLOAT diff; \
            FICL_FLOAT f1; \
            FICL_FLOAT f2; \
            VM_CHECK_STACK_LOCAL(0, 1); \
            VM_CHECK_FSTACK_LOCAL(2, 0); \
            f1 = *--FSTACK_TOP; \
            f2 = *--FSTACK_TOP; \
            diff = (FICL_FLOAT)fabs((double)(f2 - f1)); \
            dataTop->i = FICL_BOOL(diff < (FICL_FLOAT)(2 * FICL_FLOAT_EPSILON)); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FEQUAL: { \
            FICL_FLOAT x; \
            FICL_FLOAT y; \
            VM_CHECK_STACK_LOCAL(0, 1); \
            VM_CHECK_FSTACK_LOCAL(2, 0); \
            y = *--FSTACK_TOP; \
            x = *--FSTACK_TOP; \
            dataTop->i = FICL_BOOL(x == y); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FDEPTH: { \
            VM_CHECK_STACK_LOCAL(0, 1); \
            dataTop->i = (FICL_INT)(FSTACK_TOP - pVM->fStack->base); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_S_TO_F: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(0, 1); \
            *FSTACK_TOP++ = (FICL_FLOAT)(--dataTop)->i; \
            goto OP_DONE; \
        } \
        case FICL_OP_F_TO_S: { \
            VM_CHECK_STACK_LOCAL(0, 1); \
            VM_CHECK_FSTACK_LOCAL(1, 0); \
            dataTop->i = (FICL_INT)(*--FSTACK_TOP); \
            dataTop++; \
            goto OP_DONE; \
        } \
        case FICL_OP_FPLUS_I: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(1, 1); \
            FSTACK_TOP[-1] += (FICL_FLOAT)(--dataTop)->i; \
            goto OP_DONE; \
        } \
        case FICL_OP_FMINUS_I: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(1, 1); \
            FSTACK_TOP[-1] -= (FICL_FLOAT)(--dataTop)->i; \
            goto OP_DONE; \
        } \
        case FICL_OP_FSTAR_I: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(1, 1); \
            FSTACK_TOP[-1] *= (FICL_FLOAT)(--dataTop)->i; \
            goto OP_DONE; \
        } \
        case FICL_OP_FSLASH_I: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(1, 1); \
            FSTACK_TOP[-1] /= (FICL_FLOAT)(--dataTop)->i; \
            goto OP_DONE; \
        } \
        case FICL_OP_I_MINUS_F: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(1, 1); \
            FSTACK_TOP[-1] = (FICL_FLOAT)(--dataTop)->i - FSTACK_TOP[-1]; \
            goto OP_DONE; \
        } \
        case FICL_OP_I_SLASH_F: { \
            VM_CHECK_STACK_LOCAL(1, 0); \
            VM_CHECK_FSTACK_LOCAL(1, 1); \
            FSTACK_TOP[-1] = (FICL_FLOAT)(--dataTop)->i / FSTACK_TOP[-1]; \
            goto OP_DONE; \
        } \
        case FICL_OP_FCONSTANT: { \
            FICL_FLOAT _f; \
            VM_CHECK_FSTACK_LOCAL(0, 1); \
            memcpy(&_f, pWord->param, sizeof(_f)); \
            *FSTACK_TOP++ = _f; \
            goto OP_DONE; \
        }
#else
    #define VM_OP_CASES_FLOAT(OP_DONE)
#endif

#define VM_OP_CASES_IP(OP_DONE) \
    case FICL_OP_BRANCH: { \
        ip += *(int *)ip; \
        goto OP_DONE; \
    } \
    case FICL_OP_BRANCH0: { \
        VM_CHECK_STACK_LOCAL(1, 0); \
        u = (--dataTop)->u; \
        if (u) \
            ip += 1; \
        else \
            ip += *(int *)ip; \
        goto OP_DONE; \
    } \
    case FICL_OP_DO: { \
        CELL index; \
        CELL limit; \
        VM_CHECK_STACK_LOCAL(2, 0); \
        *pVM->rStack->sp++ = *(CELL *)ip; \
        ip += 1; \
        index = *--dataTop; \
        limit = *--dataTop; \
        *pVM->rStack->sp++ = limit; \
        *pVM->rStack->sp++ = index; \
        goto OP_DONE; \
    } \
    case FICL_OP_QDO: { \
        CELL index; \
        CELL limit; \
        VM_CHECK_STACK_LOCAL(2, 0); \
        *pVM->rStack->sp++ = *(CELL *)ip; \
        ip += 1; \
        index = *--dataTop; \
        limit = *--dataTop; \
        if (limit.u == index.u) { \
            ip = (IPTYPE)(--pVM->rStack->sp)->p; \
        } else { \
            *pVM->rStack->sp++ = limit; \
            *pVM->rStack->sp++ = index; \
        } \
        goto OP_DONE; \
    } \
    case FICL_OP_LOOP: { \
        FICL_INT index = pVM->rStack->sp[-1].i; \
        FICL_INT limit = pVM->rStack->sp[-2].i; \
        index++; \
        if (index >= limit) { \
            pVM->rStack->sp -= 3; \
            ip += 1; \
        } else { \
            pVM->rStack->sp[-1].i = index; \
            ip += *(int *)ip; \
        } \
        goto OP_DONE; \
    } \
    case FICL_OP_PLOOP: { \
        FICL_INT index; \
        FICL_INT limit; \
        FICL_INT increment; \
        int flag; \
        VM_CHECK_STACK_LOCAL(1, 0); \
        index = pVM->rStack->sp[-1].i; \
        limit = pVM->rStack->sp[-2].i; \
        increment = (--dataTop)->i; \
        { \
            FICL_INT oldOffset = index - limit; \
            FICL_INT newOffset = oldOffset + increment; \
            flag = ((oldOffset ^ newOffset) & (increment ^ oldOffset)) < 0; \
        } \
        index += increment; \
        if (flag) { \
            pVM->rStack->sp -= 3; \
            ip += 1; \
        } else { \
            pVM->rStack->sp[-1].i = index; \
            ip += *(int *)ip; \
        } \
        goto OP_DONE; \
    } \
    case FICL_OP_LIT: { \
        VM_CHECK_STACK_LOCAL(0, 1); \
        dataTop->i = *(FICL_INT *)ip; \
        dataTop++; \
        ip += 1; \
        goto OP_DONE; \
    } \
    case FICL_OP_2LIT: { \
        VM_CHECK_STACK_LOCAL(0, 2); \
        dataTop[0].i = ((FICL_INT *)ip)[1]; \
        dataTop[1].i = ((FICL_INT *)ip)[0]; \
        dataTop += 2; \
        ip += 2; \
        goto OP_DONE; \
    } \
    case FICL_OP_EXIT: { \
        ip = (IPTYPE)(--pVM->rStack->sp)->p; \
        goto OP_DONE; \
    } \
    case FICL_OP_SEMI: { \
        ip = (IPTYPE)(--pVM->rStack->sp)->p; \
        goto OP_DONE; \
    } \
    case FICL_OP_OF: { \
        FICL_UNS a, b; \
        VM_CHECK_STACK_LOCAL(2, 1); \
        a = (--dataTop)->u; \
        b = (dataTop - 1)->u; \
        if (a == b) { \
            dataTop--; \
            ip += 1; \
        } else { \
            ip += *(int *)ip; \
        } \
        goto OP_DONE; \
    } \
    case FICL_OP_LEAVE: { \
        pVM->rStack->sp -= 2; \
        ip = (IPTYPE)(--pVM->rStack->sp)->p; \
        goto OP_DONE; \
    } \
    case FICL_OP_UNLOOP: { \
        pVM->rStack->sp -= 3; \
        goto OP_DONE; \
    } \
    case FICL_OP_COLON: { \
        *pVM->rStack->sp++ = (CELL){.p = ip}; \
        ip = (IPTYPE)(pWord->param); \
        goto OP_DONE; \
    } \
    case FICL_OP_DOES: { \
        VM_CHECK_STACK_LOCAL(0, 1); \
        (dataTop++)->p = pWord->param + 1; \
        *pVM->rStack->sp++ = (CELL){.p = ip}; \
        ip = (IPTYPE)(pWord->param[0].p); \
        goto OP_DONE; \
    } \
    case FICL_OP_STRINGLIT: { \
        FICL_STRING *_sp = (FICL_STRING *)(ip); \
        char *_cp; \
        VM_CHECK_STACK_LOCAL(0, 2); \
        _cp = _sp->text; \
        (dataTop++)->p = _cp; \
        (dataTop++)->u = _sp->count; \
        _cp += _sp->count + 1; \
        ip = (IPTYPE)(void *)alignPtr(_cp); \
        goto OP_DONE; \
    } \
    case FICL_OP_CSTRINGLIT: { \
        FICL_STRING *_sp = (FICL_STRING *)(ip); \
        char *_cp = _sp->text; \
        _cp += _sp->count + 1; \
        ip = (IPTYPE)(void *)alignPtr(_cp); \
        VM_CHECK_STACK_LOCAL(0, 1); \
        (dataTop++)->p = _sp; \
        goto OP_DONE; \
    }

#define VM_OP_CASES_WORD(OP_DONE) \
    case FICL_OP_CONSTANT: { \
        VM_CHECK_STACK_LOCAL(0, 1); \
        *dataTop++ = pWord->param[0]; \
        goto OP_DONE; \
    } \
    case FICL_OP_2CONSTANT: { \
        VM_CHECK_STACK_LOCAL(0, 2); \
        *dataTop++ = pWord->param[0]; \
        *dataTop++ = pWord->param[1]; \
        goto OP_DONE; \
    } \
    case FICL_OP_VARIABLE: { \
        VM_CHECK_STACK_LOCAL(0, 1); \
        (dataTop++)->p = pWord->param; \
        goto OP_DONE; \
    } \
    case FICL_OP_CREATE: { \
        VM_CHECK_STACK_LOCAL(0, 1); \
        (dataTop++)->p = pWord->param + 1; \
        goto OP_DONE; \
    }

#if FICL_WANT_USER
#define VM_OP_CASES_USER(OP_DONE) \
    case FICL_OP_USER: { \
        VM_CHECK_STACK_LOCAL(0, 1); \
        (dataTop++)->p = &pVM->user[pWord->param[0].i]; \
        goto OP_DONE; \
    }
#else
#define VM_OP_CASES_USER(OP_DONE)
#endif

#define VM_OP_SWITCH_BASE(OP_DONE) \
    switch (opcode) { \
        VM_OP_CASES_BASE(OP_DONE) \
        VM_OP_CASES_FLOAT(OP_DONE) \
        VM_OP_CASES_WORD(OP_DONE) \
        VM_OP_CASES_USER(OP_DONE) \
        default: \
            break; \
    }

#define VM_OP_SWITCH_INNER(OP_DONE) \
    switch (opcode) { \
        VM_OP_CASES_BASE(OP_DONE) \
        VM_OP_CASES_FLOAT(OP_DONE) \
        VM_OP_CASES_WORD(OP_DONE) \
        VM_OP_CASES_USER(OP_DONE) \
        VM_OP_CASES_IP(OP_DONE) \
        default: \
            break; \
    }

static char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";


/**************************************************************************
                        v m B r a n c h R e l a t i v e
**
**************************************************************************/
void vmBranchRelative(FICL_VM *pVM, int offset)
{
    pVM->ip += offset;
    return;
}


/**************************************************************************
                        v m C r e a t e
** Creates a virtual machine either from scratch (if pVM is NULL on entry)
** or by resizing and reinitializing an existing VM to the specified stack
** sizes.
**************************************************************************/
FICL_VM *vmCreate(FICL_VM *pVM, unsigned nPStack, unsigned nRStack)
{
    if (pVM == NULL)
    {
        pVM = (FICL_VM *)ficlMalloc(sizeof (FICL_VM));
        assert (pVM);
        memset(pVM, 0, sizeof (FICL_VM));
    }

    if (pVM->pStack)
        stackDelete(pVM->pStack);
    pVM->pStack = stackCreate(nPStack);

    if (pVM->rStack)
        stackDelete(pVM->rStack);
    pVM->rStack = stackCreate(nRStack);

#if FICL_WANT_FLOAT
    if (pVM->fStack)
        stackDeleteFloat(pVM->fStack);
    pVM->fStack = stackCreateFloat(nPStack);
    pVM->fPrecision = 5; /* default float output precision */
#endif

    pVM->textOut = ficlTextOut;

    vmReset(pVM);
    return pVM;
}


/**************************************************************************
                        v m D e l e t e
** Free all memory allocated to the specified VM and its subordinate
** structures.
**************************************************************************/
void vmDelete (FICL_VM *pVM)
{
    if (pVM)
    {
        ficlFree(pVM->pStack);
        ficlFree(pVM->rStack);
#if FICL_WANT_FLOAT
        stackDeleteFloat(pVM->fStack);
#endif
        ficlFree(pVM);
    }

    return;
}


/**************************************************************************
                        v m E x e c u t e
** Sets up the specified word to be run by the inner interpreter.
** Executes the word's code part immediately, but in the case of
** colon definition, the definition itself needs the inner interp
** to complete. This does not happen until control reaches ficlExec
**************************************************************************/
void vmExecute(FICL_VM *pVM, FICL_WORD *pWord)
{
    pVM->runningWord = pWord;
    if (pWord->opcode != FICL_OP_CALL)
    {
        CELL *dataTop = pVM->pStack->sp;
        CELL *returnTop = pVM->rStack->sp;
#if FICL_WANT_FLOAT
        FICL_FLOAT *floatTop = pVM->fStack->sp;
#endif
        FICL_INT i;
        FICL_UNS u;
        CELL c, c2;
        FICL_OPCODE opcode = pWord->opcode;

        switch (opcode) {
            VM_OP_CASES_BASE(OP_DONE)
            VM_OP_CASES_FLOAT(OP_DONE)
            VM_OP_CASES_WORD(OP_DONE)
            VM_OP_CASES_USER(OP_DONE)
            case FICL_OP_COLON: {
                (returnTop++)->p = pVM->ip;
                pVM->ip = (IPTYPE)(pWord->param);
                goto OP_DONE;
            }
            case FICL_OP_DOES: {
                (dataTop++)->p = pWord->param + 1;
                (returnTop++)->p = pVM->ip;
                pVM->ip = (IPTYPE)(pWord->param[0].p);
                goto OP_DONE;
            }
            default:
                goto CALL_FALLBACK;
        }

OP_DONE:
        pVM->pStack->sp = dataTop;
        pVM->rStack->sp = returnTop;
#if FICL_WANT_FLOAT
        pVM->fStack->sp = floatTop;
#endif
        return;
    }

CALL_FALLBACK:
    pWord->code(pVM);
}

/**************************************************************************
                        v m S t e p
** Execute a single instruction.
**************************************************************************/
void vmStep(FICL_VM *pVM)
{
    FICL_WORD *pWord;
    CELL *dataTop = pVM->pStack->sp;
    /* returnTop removed - return stack accessed directly via pVM->rStack->sp for reduced register pressure */
#if FICL_WANT_FLOAT
    FICL_FLOAT *floatTop = pVM->fStack->sp;
#endif
    IPTYPE ip = pVM->ip;

    FICL_INT i;
    FICL_UNS u;
    CELL c, c2;
    FICL_OPCODE opcode;

    pWord = *ip++;
    pVM->runningWord = pWord;

    opcode = pWord->opcode;
    if (opcode != FICL_OP_CALL)
    {
        VM_OP_SWITCH_INNER(OP_DONE);
    }

    pVM->pStack->sp = dataTop;
    /* pVM->rStack->sp not synced - already accessed directly in operations */
#if FICL_WANT_FLOAT
    pVM->fStack->sp = floatTop;
#endif
    pVM->ip = ip;

    pWord->code(pVM);

    return;

OP_DONE:
    pVM->pStack->sp = dataTop;
    /* pVM->rStack->sp not synced - already accessed directly in operations */
#if FICL_WANT_FLOAT
    pVM->fStack->sp = floatTop;
#endif
    pVM->ip = ip;
    return;
}


/**************************************************************************
                        v m I n n e r L o o p
** The heart of the VM, this is the loop that executes instructions for colon
** definitions.
**************************************************************************/
FICL_VM_OPTIMIZE
void vmInnerLoop(FICL_VM *pVM)
{
    FICL_WORD *pWord;
    CELL *dataTop = pVM->pStack->sp;
    /* returnTop removed - return stack accessed directly via pVM->rStack->sp for reduced register pressure */
#if FICL_WANT_FLOAT
    FICL_FLOAT *floatTop = pVM->fStack->sp;
#endif
    IPTYPE ip = pVM->ip;

    FICL_INT i;
    FICL_UNS u;
    CELL c, c2;
    FICL_OPCODE opcode;

    for (;;)
    {
        pWord = *ip++;
        pVM->runningWord = pWord;

        opcode = pWord->opcode;
        if (opcode != FICL_OP_CALL)
        {
            VM_OP_SWITCH_INNER(OP_CONTINUE);
        }

        pVM->pStack->sp = dataTop;
        /* pVM->rStack->sp not synced - already accessed directly in operations */
#if FICL_WANT_FLOAT
        pVM->fStack->sp = floatTop;
#endif
        pVM->ip = ip;

        pWord->code(pVM);

        dataTop = pVM->pStack->sp;
        /* returnTop removed - no need to reload */
#if FICL_WANT_FLOAT
        floatTop = pVM->fStack->sp;
#endif
        ip = pVM->ip;
        continue;

OP_CONTINUE:
        continue;
    }
}

/**************************************************************************
                        v m G e t D i c t
** Returns the address dictionary for this VM's system
**************************************************************************/
FICL_DICT  *vmGetDict(FICL_VM *pVM)
{
	assert(pVM);
	return pVM->pSys->dp;
}


/**************************************************************************
                        v m G e t S t r i n g
** Parses a string out of the VM input buffer and copies up to the first
** FICL_STRING_MAX characters to the supplied destination buffer, a
** FICL_STRING. The destination string is NULL terminated.
**
** Returns the address of the first unused character in the dest buffer.
**************************************************************************/
char *vmGetString(FICL_VM *pVM, FICL_STRING *spDest, char delimiter)
{
    STRINGINFO si = vmParseStringEx(pVM, delimiter, 0);

    if (SI_COUNT(si) > FICL_STRING_MAX)
    {
        SI_SETLEN(si, FICL_STRING_MAX);
    }

    strncpy(spDest->text, SI_PTR(si), SI_COUNT(si));
    spDest->text[SI_COUNT(si)] = '\0';
    spDest->count = (FICL_COUNT)SI_COUNT(si);

    return spDest->text + SI_COUNT(si) + 1;
}


/**************************************************************************
                        v m G e t W o r d
** vmGetWord calls vmGetWord0 repeatedly until it gets a string with
** non-zero length.
**************************************************************************/
STRINGINFO vmGetWord(FICL_VM *pVM)
{
    STRINGINFO si = vmGetWord0(pVM);

    if (SI_COUNT(si) == 0)
    {
        vmThrow(pVM, VM_RESTART);
    }

    return si;
}


/**************************************************************************
                        v m G e t W o r d 0
** Skip leading whitespace and parse a space delimited word from the tib.
** Returns the start address and length of the word. Updates the tib
** to reflect characters consumed, including the trailing delimiter.
** If there's nothing of interest in the tib, returns zero. This function
** does not use vmParseString because it uses isspace() rather than a
** single  delimiter character.
**************************************************************************/
STRINGINFO vmGetWord0(FICL_VM *pVM)
{
    const char *pSrc = vmGetInBuf(pVM);
    const char *pEnd = vmGetInBufEnd(pVM);
    STRINGINFO si;
    FICL_UNS count = 0;
    char ch = 0;

    pSrc = skipSpace(pSrc, pEnd);
    SI_SETPTR(si, pSrc);

#if 0
    for (ch = *pSrc; (pEnd != pSrc) && !isspace(ch); ch = *++pSrc)
    {
        count++;
    }
#else
    /* Changed to make Purify happier.  --lch */
    for (;;)
    {
        if (pEnd == pSrc)
            break;
        ch = *pSrc;
        if (isspace(ch))
            break;
        count++;
        pSrc++;
    }
#endif

    SI_SETLEN(si, count);

    if ((pEnd != pSrc) && isspace(ch))    /* skip one trailing delimiter */
        pSrc++;

    vmUpdateTib(pVM, pSrc);

    return si;
}


/**************************************************************************
                        v m G e t W o r d T o P a d
** Does vmGetWord and copies the result to the pad as a NULL terminated
** string. Returns the length of the string. If the string is too long
** to fit in the pad, it is truncated.
**************************************************************************/
int vmGetWordToPad(FICL_VM *pVM)
{
    STRINGINFO si;
    char *cp = (char *)pVM->pad;
    si = vmGetWord(pVM);

    if (SI_COUNT(si) > nPAD-1)
        SI_SETLEN(si, nPAD-1);

    strncpy(cp, SI_PTR(si), SI_COUNT(si));
    cp[SI_COUNT(si)] = '\0';
    return (int)(SI_COUNT(si));
}


/**************************************************************************
                        v m P a r s e S t r i n g
** Parses a string out of the input buffer using the delimiter
** specified. Skips leading delimiters, marks the start of the string,
** and counts characters to the next delimiter it encounters. It then
** updates the vm input buffer to consume all these chars, including the
** trailing delimiter.
** Returns the address and length of the parsed string, not including the
** trailing delimiter.
**************************************************************************/
STRINGINFO vmParseString(FICL_VM *pVM, char delim)
{
    return vmParseStringEx(pVM, delim, 1);
}

STRINGINFO vmParseStringEx(FICL_VM *pVM, char delim, char fSkipLeading)
{
    STRINGINFO si;
    const char *pSrc = vmGetInBuf(pVM);
    const char *pEnd = vmGetInBufEnd(pVM);
    char ch;

    if (fSkipLeading)
    {                       /* skip lead delimiters */
        while ((pSrc != pEnd) && (*pSrc == delim))
            pSrc++;
    }

    SI_SETPTR(si, pSrc);    /* mark start of text */

    for (ch = *pSrc; (pSrc != pEnd)
                  && (ch != delim)
                  && (ch != '\r')
                  && (ch != '\n'); ch = *++pSrc)
    {
        ;                   /* find next delimiter or end of line */
    }

                            /* set length of result */
    SI_SETLEN(si, pSrc - SI_PTR(si));

    if ((pSrc != pEnd) && (*pSrc == delim))     /* gobble trailing delimiter */
        pSrc++;

    vmUpdateTib(pVM, pSrc);
    return si;
}


/**************************************************************************
                        v m P o p
**
**************************************************************************/
CELL vmPop(FICL_VM *pVM)
{
    return stackPop(pVM->pStack);
}


/**************************************************************************
                        v m P u s h
**
**************************************************************************/
void vmPush(FICL_VM *pVM, CELL c)
{
    stackPush(pVM->pStack, c);
    return;
}


/**************************************************************************
                        v m P o p I P
**
**************************************************************************/
void vmPopIP(FICL_VM *pVM)
{
    pVM->ip = (IPTYPE)(stackPopPtr(pVM->rStack));
    return;
}


/**************************************************************************
                        v m P u s h I P
**
**************************************************************************/
void vmPushIP(FICL_VM *pVM, IPTYPE newIP)
{
    stackPushPtr(pVM->rStack, (void *)pVM->ip);
    pVM->ip = newIP;
    return;
}


/**************************************************************************
                        v m P u s h T i b
** Binds the specified input string to the VM and clears >IN (the index)
**************************************************************************/
void vmPushTib(FICL_VM *pVM, const char *text, FICL_INT nChars, TIB *pSaveTib)
{
    if (pSaveTib)
    {
        *pSaveTib = pVM->tib;
    }

    pVM->tib.cp = text;
    pVM->tib.end = text + nChars;
    pVM->tib.index = 0;
}


void vmPopTib(FICL_VM *pVM, TIB *pTib)
{
    if (pTib)
    {
        pVM->tib = *pTib;
    }
    return;
}


/**************************************************************************
                        v m Q u i t
**
**************************************************************************/
void vmQuit(FICL_VM *pVM)
{
    stackReset(pVM->rStack);
    pVM->fRestart    = 0;
    pVM->ip          = NULL;
    pVM->runningWord = NULL;
    pVM->state       = INTERPRET;
    pVM->tib.cp      = NULL;
    pVM->tib.end     = NULL;
    pVM->tib.index   = 0;
    pVM->pad[0]      = '\0';
    pVM->sourceID.i  = 0;
    return;
}


/**************************************************************************
                        v m R e s e t
**
**************************************************************************/
void vmReset(FICL_VM *pVM)
{
    vmQuit(pVM);
    stackReset(pVM->pStack);
#if FICL_WANT_FLOAT
    stackResetFloat(pVM->fStack);
#endif
    pVM->base        = 10;
    return;
}


/**************************************************************************
                        v m S e t T e x t O u t
** Binds the specified output callback to the vm. If you pass NULL,
** binds the default output function (ficlTextOut)
**************************************************************************/
void vmSetTextOut(FICL_VM *pVM, OUTFUNC textOut)
{
    if (textOut)
        pVM->textOut = textOut;
    else
        pVM->textOut = ficlTextOut;

    return;
}


/**************************************************************************
                        v m T e x t O u t
** Feeds text to the vm's output callback
**************************************************************************/
void vmTextOut(FICL_VM *pVM, const char *text, int fNewline)
{
    assert(pVM);
    assert(pVM->textOut);
    (pVM->textOut)(pVM, text, fNewline);

    return;
}


/**************************************************************************
                        v m T h r o w
**
**************************************************************************/
void vmThrow(FICL_VM *pVM, int except)
{
    if (pVM->pState)
        FICL_LONGJMP(*(pVM->pState), except);
}

void vmThrowErr(FICL_VM *pVM, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vsnprintf(pVM->pad, sizeof(pVM->pad), fmt, va);
    vmTextOut(pVM, pVM->pad, 1);
    va_end(va);
    FICL_LONGJMP(*(pVM->pState), VM_ERREXIT);
}

void vmThrowOverflow(FICL_VM *pVM)
{
    vmTextOut(pVM, "Error: Stack overflow", 1);
    FICL_LONGJMP(*(pVM->pState), VM_ERREXIT);
}

void vmThrowUnderflow(FICL_VM *pVM)
{
    vmTextOut(pVM, "Error: Stack underflow", 1);
    FICL_LONGJMP(*(pVM->pState), VM_ERREXIT);
}


/**************************************************************************
                        v m I n t e r r u p t
** Interrupt the VM's inner loop from an external context (signal handler,
** hardware interrupt, timer ISR, etc.). Causes the VM to longjmp back to
** the nearest exception recovery point with VM_INTERRUPT.
** Safe to call from POSIX signal handlers on targets that use siglongjmp.
**************************************************************************/
void vmInterrupt(FICL_VM *pVM)
{
    if (pVM->pState)
        FICL_LONGJMP(*(pVM->pState), VM_INTERRUPT);
}

/**************************************************************************
                        w o r d I s I m m e d i a t e
**
**************************************************************************/
int wordIsImmediate(FICL_WORD *pFW)
{
    return ((pFW != NULL) && (pFW->flags & FW_IMMEDIATE));
}


/**************************************************************************
                        w o r d I s C o m p i l e O n l y
**
**************************************************************************/
int wordIsCompileOnly(FICL_WORD *pFW)
{
    return ((pFW != NULL) && (pFW->flags & FW_COMPILE));
}


/**************************************************************************
                        s t r r e v
**
**************************************************************************/
char *ficlStrrev(char *string )
{                               /* reverse a string in-place */
    int i = strlen(string);
    char *p1 = string;          /* first char of string */
    char *p2 = string + i - 1;  /* last non-NULL char of string */
    char c;

    if (i > 1)
    {
        while (p1 < p2)
        {
            c = *p2;
            *p2 = *p1;
            *p1 = c;
            p1++; p2--;
        }
    }

    return string;
}


/**************************************************************************
                        d i g i t _ t o _ c h a r
**
**************************************************************************/
char digit_to_char(int value)
{
    return digits[value];
}


/**************************************************************************
                        i s P o w e r O f T w o
** Tests whether supplied argument is an integer power of 2 (2**n)
** where 32 > n > 1, and returns n if so. Otherwise returns zero.
**************************************************************************/
int isPowerOfTwo(FICL_UNS u)
{
    int i = 1;
    FICL_UNS t = 2;

    for (; ((t <= u) && (t != 0)); i++, t <<= 1)
    {
        if (u == t)
            return i;
    }

    return 0;
}


/**************************************************************************
                        l t o a   &   u l t o a
**
**************************************************************************/
static char *ltoaFactor( FICL_INT value, char *string, int radix )
{
    char *cp = string;
    int pwr = isPowerOfTwo((FICL_UNS)radix);
    assert(radix > 1);
    assert(radix < 37);
    assert(string);

    if (value == 0)
        *cp++ = '0';
    else if (pwr != 0)
    {
        FICL_UNS v = (FICL_UNS) value;
        FICL_UNS mask = (FICL_UNS) ~(-1 << pwr);
        while (v)
        {
            *cp++ = digits[v & mask];
            v >>= pwr;
        }
    }
    else
    {
        UNSQR result;
        DPUNS v;
        v.hi = 0;
        v.lo = (FICL_UNS)value;
        while (v.lo)
        {
            result = ficlLongDiv(v, (FICL_UNS)radix);
            *cp++ = digits[result.rem];
            v.lo = result.quot;
        }
    }

    *cp = '\0';
    return cp;
}


char *ficlLtoa( FICL_INT value, char *string, int radix )
{                               /* convert long to string, any base */
    int isNeg = (value < 0);
    if (isNeg)
        value = -value;

    char *cp = ltoaFactor( value, string, radix );

    if (isNeg)
        *cp++ = '-';

    *cp++ = '\0';

    return ficlStrrev(string);
}


char *ficlUltoa(FICL_UNS value, char *string, int radix )
{                               /* convert long to string, any base */
    char *cp = ltoaFactor( value, string, radix );
    *cp++ = '\0';
    return ficlStrrev(string);
}


/**************************************************************************
                        s t r i n c m p
** (jws) simplified the code a bit in hopes of appeasing Purify
**************************************************************************/
int strincmp(const char *cp1, const char *cp2, FICL_UNS count)
{
    int i = 0;

    for (; 0 < count; ++cp1, ++cp2, --count)
    {
        i = tolower(*cp1) - tolower(*cp2);
        if (i != 0)
            return i;
        else if (*cp1 == '\0')
            return 0;
    }
    return 0;
}

/**************************************************************************
                        s k i p S p a c e
** Given a string pointer, returns a pointer to the first non-space
** char of the string, or to the NULL terminator if no such char found.
** If the pointer reaches "end" first, stop there. Pass NULL to
** suppress this behavior.
**************************************************************************/
const char *skipSpace(const char *cp, const char *end)
{
    assert(cp);

    while ((cp != end) && isspace(*cp))
        cp++;

    return cp;
}
