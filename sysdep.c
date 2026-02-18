/*******************************************************************
** s y s d e p . c
** Forth Inspired Command Language
** Author: John W Sadler
** Created: 16 Oct 1997
** Implementations of FICL external interface functions...
**
** (simple) port to Linux, Skip Carter 26 March 1998
** $Id: sysdep.c,v 1.9 2001-07-23 22:01:24-07 jsadler Exp jsadler $
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
#include "ficl.h"

/*
**                S T A T I C   A S S E R T S
** Verify that sizes of key scalar data types match expectations
*/
static_assert(CHAR_BIT == 8,                        "CHAR_BIT size issue");
static_assert(sizeof(UNS16) == 2,                   "UNS16 size issue");
static_assert(sizeof(INT32) == 4,                   "INT32 must be 4 bytes");
static_assert(sizeof(UNS32) == 4,                   "UNS32 must be 4 bytes");
static_assert(sizeof(FICL_INT) == sizeof(void *),   "FICL_INT must be pointer sized");
static_assert(sizeof(FICL_UNS) == sizeof(void *),   "FICL_UNS must be pointer sized");

/*
**                 M A C O S   P O R T   B E G I N S
*/
#if defined(MACOS)
#include <unistd.h>

void  ficlTextOut(FICL_VM *pVM, const char *msg, int fNewline)
{
    IGNORE(pVM);

    while(*msg != 0)
    putchar(*(msg++));
    if (fNewline)
    putchar('\n');

   return;
}

void *ficlMalloc (size_t size)
{
    return malloc(size);
}

void *ficlRealloc (void *p, size_t size)
{
    return realloc(p, size);
}

void  ficlFree   (void *p)
{
    free(p);
}

#if ! PORTABLE_LONGMULDIV
    DPUNS ficlLongMul(FICL_UNS x, FICL_UNS y)
    {
        unsigned __int128 prod = (unsigned __int128)x * (unsigned __int128)y;
        DPUNS result;

        /* low half = bottom N bits */
        result.lo = (FICL_UNS)prod;
        /* high half = top N bits */
        result.hi = (FICL_UNS)(prod >> (sizeof(FICL_UNS) * CHAR_BIT));

        return result;
    }

    UNSQR ficlLongDiv(DPUNS dividend, FICL_UNS y)
    {
        unsigned __int128 numerator =
           ((unsigned __int128)dividend.hi << (sizeof(FICL_UNS) * CHAR_BIT))
          | (unsigned __int128)dividend.lo;

        unsigned __int128 q128 = numerator / y;
        unsigned __int128 r128 = numerator % y;

        UNSQR result;
        result.quot = (FICL_UNS)q128;
        result.rem  = (FICL_UNS)r128;
        return result;
    }
#endif /* PORTABLE_LONGMULDIV */

/*
*******************  FreeBSD  P O R T   B E G I N S   H E R E ******************** Michael Smith
*/
#elif defined (FREEBSD_ALPHA)
#if ! PORTABLE_LONGMULDIV
    DPUNS ficlLongMul(FICL_UNS x, FICL_UNS y)
    {
        DPUNS q;
        u_int64_t qx;

        qx = (u_int64_t)x * (u_int64_t) y;

        q.hi = (u_int32_t)( qx >> 32 );
        q.lo = (u_int32_t)( qx & 0xFFFFFFFFL);

        return q;
    }

    UNSQR ficlLongDiv(DPUNS q, FICL_UNS y)
    {
        UNSQR result;
        u_int64_t qx, qh;

        qh = q.hi;
        qx = (qh << 32) | q.lo;

        result.quot = qx / y;
        result.rem  = qx % y;

        return result;
    }
#endif

void  ficlTextOut(FICL_VM *pVM, const char *msg, int fNewline)
{
    IGNORE(pVM);

    while(*msg != 0)
    putchar(*(msg++));
    if (fNewline)
    putchar('\n');

   return;
}

void *ficlMalloc (size_t size)
{
    return malloc(size);
}

void *ficlRealloc (void *p, size_t size)
{
    return realloc(p, size);
}

void  ficlFree   (void *p)
{
    free(p);
}


/*
*******************  P C / W I N 3 2   P O R T   B E G I N S   H E R E ***********************
*/
#elif defined (_M_IX86)
#if ! PORTABLE_LONGMULDIV
    DPUNS ficlLongMul(FICL_UNS x, FICL_UNS y)
    {
        DPUNS q;

        __asm
        {
            mov eax,x
            mov edx,y
            mul edx
            mov q.hi,edx
            mov q.lo,eax
        }

        return q;
    }

    UNSQR ficlLongDiv(DPUNS q, FICL_UNS y)
    {
        UNSQR result;

        __asm
        {
            mov eax,q.lo
            mov edx,q.hi
            div y
            mov result.quot,eax
            mov result.rem,edx
        }

        return result;
    }
#endif

#if !defined (_WINDOWS)
    void ficlTextOut(FICL_VM *pVM, const char *msg, int fNewline)
    {
        IGNORE(pVM);

        if (fNewline)
            puts(msg);
        else
            fputs(msg, stdout);

       return;
    }
#endif

void *ficlMalloc (size_t size)
{
    return malloc(size);
}


void  ficlFree   (void *p)
{
    free(p);
}


void *ficlRealloc(void *p, size_t size)
{
    return realloc(p, size);
}


/*
*******************  6 8 K  C P U 3 2  P O R T   B E G I N S   H E R E ********************
*/
#elif defined (MOTO_CPU32)

#if ! PORTABLE_LONGMULDIV
    DPUNS ficlLongMul(FICL_UNS x, FICL_UNS y)
    {
        DPUNS q;
        IGNORE(q);    /* suppress goofy compiler warnings */
        IGNORE(x);
        IGNORE(y);

    #pragma ASM
        move.l (S_x,a6),d1
        mulu.l (S_y,a6),d0:d1
        move.l d1,(S_q+4,a6)
        move.l d0,(S_q+0,a6)
    #pragma END_ASM

        return q;
    }

    UNSQR ficlLongDiv(DPUNS q, FICL_UNS y)
    {
        UNSQR result;
        IGNORE(result); /* suppress goofy compiler warnings */
        IGNORE(q);
        IGNORE(y);

    #pragma ASM
        move.l (S_q+0,a6),d0    ; hi 32 --> d0
        move.l (S_q+4,a6),d1    ; lo 32 --> d1
        divu.l (S_y,a6),d0:d1   ; d0 <-- rem, d1 <-- quot
        move.l d1,(S_result+0,a6)
        move.l d0,(S_result+4,a6)
    #pragma END_ASM

        return result;
    }
#endif

void  ficlTextOut(FICL_VM *pVM, const char *msg, int fNewline)
{
   return;
}

void *ficlMalloc (size_t size)
{
}

void  ficlFree   (void *p)
{
}


void *ficlRealloc(void *p, size_t size)
{
    void *pv = malloc(size);
    if (p)
    {
        memcpy(pv, p, size)
        free(p);
    }

    return pv;
}

#endif /* MOTO_CPU32 */

/*
*******************  Linux  P O R T   B E G I N S   H E R E ******************** Skip Carter, March 1998
*/

#if defined(linux) || defined(riscos) || defined(_WIN64)

#if ! PORTABLE_LONGMULDIV

    typedef unsigned long long __udp;
    typedef unsigned long __u32;

    DPUNS ficlLongMul(FICL_UNS x, FICL_UNS y)
    {
        DPUNS q;
        __udp qx;

        qx = (__udp)x * (__udp) y;

        q.hi = (__u32)( qx >> 32 );
        q.lo = (__u32)( qx & 0xFFFFFFFFL);

        return q;
    }

    UNSQR ficlLongDiv(DPUNS q, FICL_UNS y)
    {
        UNSQR result;
        __udp qx, qh;

        qh = q.hi;
        qx = (qh << 32) | q.lo;

        result.quot = qx / y;
        result.rem  = qx % y;

        return result;
    }

#endif

void  ficlTextOut(FICL_VM *pVM, const char *msg, int fNewline)
{
    IGNORE(pVM);

    if (fNewline)
        puts(msg);
    else
        fputs(msg, stdout);

   return;
}

void *ficlMalloc (size_t size)
{
    return malloc(size);
}

void  ficlFree   (void *p)
{
    free(p);
}

void *ficlRealloc(void *p, size_t size)
{
    return realloc(p, size);
}

#endif /* linux */


/*
** D E P R E C A T E D   F I C L _ M U L T I S E S S I O N
** Never implemented to my knowledge, and should refer to multi-session
** rather than multithread.
**
** Stub function for dictionary access control - does nothing
** by default, user can redefine to guarantee exclusive dict
** access to a single thread for updates. All dict update code
** is guaranteed to be bracketed as follows:
** ficlLockDictionary(TRUE);
** <code that updates dictionary>
** ficlLockDictionary(FALSE);
**
** Returns zero if successful, nonzero if unable to acquire lock
** before timeout (optional - could also block forever)
*/
#if FICL_MULTISESSION
int ficlLockDictionary(short fLock)
{
    IGNORE(fLock);
    return 0;
}
#endif /* FICL_MULTISESSION */


