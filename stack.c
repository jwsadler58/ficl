/*******************************************************************
** s t a c k . c
** Forth Inspired Command Language
** Author: John W Sadler
** Created: 16 Oct 1997
** $Id: stack.c,v 1.7 2001-06-12 01:24:35-07 jsadler Exp jsadler $
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

#include "ficl.h"

#define STKDEPTH(s) ((s)->sp - (s)->base)
#define FSTKDEPTH(s) ((s)->sp - (s)->base)

/*
** N O T E: Stack convention:
**
** sp points to the first available cell
** push: store value at sp, increment sp
** pop:  decrement sp, fetch value at sp
** Stack grows from low to high memory
*/

/*******************************************************************
                    v m C h e c k S t a c k
** Check the parameter stack for underflow or overflow.
** nCells controls the type of check: if nCells is zero,
** the function checks the stack state for underflow and overflow.
** If nCells > 0, checks to see that the stack has room to push
** that many cells. If less than zero, checks to see that the
** stack has room to pop that many cells. If any test fails,
** the function throws (via vmThrow) a VM_ERREXIT exception.
*******************************************************************/
void vmCheckStack(FICL_VM *pVM, int popCells, int pushCells)
{
    FICL_STACK *pStack = pVM->pStack;
    int nFree = pStack->base + pStack->nCells - pStack->sp;

    if (popCells > STKDEPTH(pStack))
    {
        vmThrowErr(pVM, "Error: stack underflow");
    }

    if (nFree < pushCells - popCells)
    {
        vmThrowErr(pVM, "Error: stack overflow");
    }

    return;
}

#if FICL_WANT_FLOAT
void vmCheckFStack(FICL_VM *pVM, int popCells, int pushCells)
{
    FICL_FSTACK *fStack = pVM->fStack;
    int nFree = (int)(fStack->base + fStack->nCells - fStack->sp);

    if (popCells > FSTKDEPTH(fStack))
    {
        vmThrowErr(pVM, "Error: float stack underflow");
    }

    if (nFree < pushCells - popCells)
    {
        vmThrowErr(pVM, "Error: float stack overflow");
    }
}
#endif

/*******************************************************************
                    s t a c k C r e a t e
**
*******************************************************************/

FICL_STACK *stackCreate(unsigned nCells)
{
    size_t size = FICL_STACK_BYTES(nCells);
    FICL_STACK *pStack = ficlMalloc(size);

#if FICL_ROBUST
    assert (nCells != 0);
    assert (pStack != NULL);
#endif

    pStack->nCells = nCells;
    pStack->sp     = pStack->base;
    pStack->pFrame = NULL;
    return pStack;
}

#if FICL_WANT_FLOAT
/*******************************************************************
                    s t a c k C r e a t e F l o a t
**
*******************************************************************/
FICL_FSTACK *stackCreateFloat(unsigned nCells)
{
    size_t size = FICL_FSTACK_BYTES(nCells);
    FICL_FSTACK *pStack = ficlMalloc(size);

#if FICL_ROBUST
    assert (nCells != 0);
    assert (pStack != NULL);
#endif

    pStack->nCells = nCells;
    pStack->sp     = pStack->base;
    return pStack;
}
#endif

/*******************************************************************
                    s t a c k D e l e t e
**
*******************************************************************/

void stackDelete(FICL_STACK *pStack)
{
    if (pStack)
        ficlFree(pStack);
    return;
}

#if FICL_WANT_FLOAT
/*******************************************************************
                    s t a c k D e l e t e F l o a t
**
*******************************************************************/
void stackDeleteFloat(FICL_FSTACK *pStack)
{
    if (pStack)
        ficlFree(pStack);
    return;
}
#endif

/*******************************************************************
                    s t a c k D e p t h
**
*******************************************************************/

int stackDepth(FICL_STACK *pStack)
{
    return STKDEPTH(pStack);
}

#if FICL_WANT_FLOAT
/*******************************************************************
                    s t a c k D e p t h F l o a t
**
*******************************************************************/
int stackDepthFloat(FICL_FSTACK *pStack)
{
    return FSTKDEPTH(pStack);
}
#endif
/*******************************************************************
                    s t a c k D r o p
**
*******************************************************************/

void stackDrop(FICL_STACK *pStack, int n)
{
#if FICL_ROBUST
    assert(n > 0);
#endif
    pStack->sp -= n;
    return;
}

#if FICL_WANT_FLOAT
/*******************************************************************
                    s t a c k D r o p F l o a t
**
*******************************************************************/
void stackDropFloat(FICL_FSTACK *pStack, int n)
{
#if FICL_ROBUST
    assert(n > 0);
#endif
    pStack->sp -= n;
    return;
}
#endif

/*******************************************************************
                    s t a c k F e t c h
**
*******************************************************************/

CELL stackFetch(FICL_STACK *pStack, int n)
{
    return pStack->sp[-n-1];
}

void stackStore(FICL_STACK *pStack, int n, CELL c)
{
    pStack->sp[-n-1] = c;
    return;
}


/*******************************************************************
                    s t a c k G e t T o p
**
*******************************************************************/

CELL stackGetTop(FICL_STACK *pStack)
{
    return pStack->sp[-1];
}

#if FICL_WANT_FLOAT
/*******************************************************************
                    s t a c k G e t T o p F l o a t
**
*******************************************************************/
FICL_FLOAT stackGetTopFloat(FICL_FSTACK *pStack)
{
    return pStack->sp[-1];
}

/*******************************************************************
                    s t a c k S e t T o p F l o a t
**
*******************************************************************/
void stackSetTopFloat(FICL_FSTACK *pStack, FICL_FLOAT f)
{
    pStack->sp[-1] = f;
    return;
}
#endif

/*******************************************************************
                    s t a c k L i n k
** Link a frame using the stack's frame pointer. Allot space for
** nCells cells in the frame
** 1) Push pFrame
** 2) pFrame = sp
** 3) sp += nCells
*******************************************************************/

void stackLink(FICL_STACK *pStack, int nCells)
{
    stackPushPtr(pStack, pStack->pFrame);
    pStack->pFrame = pStack->sp;
    pStack->sp += nCells;
    return;
}


/*******************************************************************
                    s t a c k U n l i n k
** Unink a stack frame previously created by stackLink
** 1) sp = pFrame
** 2) pFrame = pop()
*******************************************************************/

void stackUnlink(FICL_STACK *pStack)
{
    pStack->sp = pStack->pFrame;
    pStack->pFrame = stackPopPtr(pStack);
    return;
}


/*******************************************************************
                    s t a c k P i c k
**
*******************************************************************/

void stackPick(FICL_STACK *pStack, int n)
{
    stackPush(pStack, stackFetch(pStack, n));
    return;
}

#if FICL_WANT_FLOAT
/*******************************************************************
                    s t a c k P i c k F l o a t
**
*******************************************************************/
void stackPickFloat(FICL_FSTACK *pStack, int n)
{
    stackPushFloat(pStack, pStack->sp[-n-1]);
    return;
}
#endif

/*******************************************************************
                    s t a c k P o p
**
*******************************************************************/

CELL stackPop(FICL_STACK *pStack)
{
    return *--pStack->sp;
}

void *stackPopPtr(FICL_STACK *pStack)
{
    return (*--pStack->sp).p;
}

FICL_UNS stackPopUNS(FICL_STACK *pStack)
{
    return (*--pStack->sp).u;
}

FICL_INT stackPopINT(FICL_STACK *pStack)
{
    return (*--pStack->sp).i;
}

#if (FICL_WANT_FLOAT)
FICL_FLOAT stackPopFloat(FICL_FSTACK *pStack)
{
    return *--pStack->sp;
}
#endif

/*******************************************************************
                    s t a c k P u s h
**
*******************************************************************/

void stackPush(FICL_STACK *pStack, CELL c)
{
    *pStack->sp++ = c;
}

void stackPushPtr(FICL_STACK *pStack, const void *ptr)
{
    *pStack->sp++ = LVALUEtoCELL(ptr);
}

void stackPushUNS(FICL_STACK *pStack, FICL_UNS u)
{
    *pStack->sp++ = LVALUEtoCELL(u);
}

void stackPushINT(FICL_STACK *pStack, FICL_INT i)
{
    *pStack->sp++ = LVALUEtoCELL(i);
}

#if (FICL_WANT_FLOAT)
void stackPushFloat(FICL_FSTACK *pStack, FICL_FLOAT f)
{
    *pStack->sp++ = f;
}
#endif

/*******************************************************************
                    s t a c k R e s e t
**
*******************************************************************/

void stackReset(FICL_STACK *pStack)
{
    pStack->sp = pStack->base;
    return;
}

#if FICL_WANT_FLOAT
/*******************************************************************
                    s t a c k R e s e t F l o a t
**
*******************************************************************/
void stackResetFloat(FICL_FSTACK *pStack)
{
    pStack->sp = pStack->base;
    return;
}
#endif

/*******************************************************************
                    s t a c k R o l l
** Roll nth stack entry to the top (counting from zero), if n is
** >= 0. Drop other entries as needed to fill the hole.
** If n < 0, roll top-of-stack to nth entry, pushing others
** upward as needed to fill the hole.
*******************************************************************/

void stackRoll(FICL_STACK *pStack, int n)
{
    CELL c;
    CELL *pCell;

    if (n == 0)
        return;
    else if (n > 0)
    {
        pCell = pStack->sp - n - 1;
        c = *pCell;

        for (;n > 0; --n, pCell++)
        {
            *pCell = pCell[1];
        }

        *pCell = c;
    }
    else
    {
        pCell = pStack->sp - 1;
        c = *pCell;

        for (; n < 0; ++n, pCell--)
        {
            *pCell = pCell[-1];
        }

        *pCell = c;
    }
    return;
}

#if FICL_WANT_FLOAT
/*******************************************************************
                    s t a c k R o l l F l o a t
** Roll nth stack entry to the top (counting from zero), if n is
** >= 0. Drop other entries as needed to fill the hole.
** If n < 0, roll top-of-stack to nth entry, pushing others
** upward as needed to fill the hole.
*******************************************************************/
void stackRollFloat(FICL_FSTACK *pStack, int n)
{
    FICL_FLOAT f;
    FICL_FLOAT *pFloat;

    if (n == 0)
        return;
    else if (n > 0)
    {
        pFloat = pStack->sp - n - 1;
        f = *pFloat;

        for (;n > 0; --n, pFloat++)
        {
            *pFloat = pFloat[1];
        }

        *pFloat = f;
    }
    else
    {
        pFloat = pStack->sp - 1;
        f = *pFloat;

        for (; n < 0; ++n, pFloat--)
        {
            *pFloat = pFloat[-1];
        }

        *pFloat = f;
    }
    return;
}
#endif

/*******************************************************************
                    s t a c k S e t T o p
**
*******************************************************************/

void stackSetTop(FICL_STACK *pStack, CELL c)
{
    pStack->sp[-1] = c;
    return;
}
