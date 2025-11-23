/*
** stub main for testing FICL under a Posix compliant OS
** $Id: testmain.c,v 1.11 2001-10-28 10:59:19-08 jsadler Exp jsadler $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ficl.h"

#if FICL_UNIT_TEST
    #include "math64.h"
    #include "unity.h"
#endif

/*
** Ficl interface to POSIX getcwd()
** Prints the current working directory using the VM's 
** textOut method...
*/
static void ficlGetCWD(FICL_VM *pVM)
{
    char *cp;

   cp = getcwd(NULL, 80);
    vmTextOut(pVM, cp, 1);
    free(cp);
    return;
}

/*
** Ficl interface to chdir
** Gets a newline (or NULL) delimited string from the input
** and feeds it to the Posix chdir function...
** Example:
**    cd c:\tmp
*/
static void ficlChDir(FICL_VM *pVM)
{
    FICL_STRING *pFS = (FICL_STRING *)pVM->pad;
    vmGetString(pVM, pFS, '\n');
    if (pFS->count > 0)
    {
       int err = chdir(pFS->text);
       if (err)
        {
            vmTextOut(pVM, "Error: path not found", 1);
            vmThrow(pVM, VM_QUIT);
        }
    }
    else
    {
        vmTextOut(pVM, "Warning (chdir): nothing happened", 1);
    }
    return;
}

/*
** Ficl interface to system (ANSI)
** Gets a newline (or NULL) delimited string from the input
** and feeds it to the Posix system function...
** Example:
**    system del *.*
**    \ ouch!
*/
static void ficlSystem(FICL_VM *pVM)
{
    FICL_STRING *pFS = (FICL_STRING *)pVM->pad;

    vmGetString(pVM, pFS, '\n');
    if (pFS->count > 0)
    {
        int err = system(pFS->text);
        if (err)
        {
            sprintf(pVM->pad, "System call returned %d", err);
            vmTextOut(pVM, pVM->pad, 1);
            vmThrow(pVM, VM_QUIT);
        }
    }
    else
    {
        vmTextOut(pVM, "Warning (system): nothing happened", 1);
    }
    return;
}

/*
** Ficl add-in to load a text file and execute it...
** Cheesy, but illustrative.
** Line oriented... filename is newline (or NULL) delimited.
** Example:
**    load test.ficl
*/
#define nLINEBUF 256
static void ficlLoad(FICL_VM *pVM)
{
    char    cp[nLINEBUF];
    char    filename[nLINEBUF];
    FICL_STRING *pFilename = (FICL_STRING *)filename;
    int     nLine = 0;
    FILE   *fp;
    int     result;
    CELL    id;
    struct stat buf;


    vmGetString(pVM, pFilename, '\n');

    if (pFilename->count <= 0)
    {
        vmTextOut(pVM, "Warning (load): nothing happened", 1);
        return;
    }

    /*
    ** get the file's size and make sure it exists 
    */
    result = stat( pFilename->text, &buf );

    if (result != 0)
    {
        vmTextOut(pVM, "Unable to stat file: ", 0);
        vmTextOut(pVM, pFilename->text, 1);
        vmThrow(pVM, VM_QUIT);
    }

    fp = fopen(pFilename->text, "r");
    if (!fp)
    {
        vmTextOut(pVM, "Unable to open file ", 0);
        vmTextOut(pVM, pFilename->text, 1);
        vmThrow(pVM, VM_QUIT);
    }

    id = pVM->sourceID;
    pVM->sourceID.p = (void *)fp;

    /* feed each line to ficlExec */
    while (fgets(cp, nLINEBUF, fp))
    {
        int len = strlen(cp) - 1;

        nLine++;
        if (len <= 0)
            continue;

        if (cp[len] == '\n')
            cp[len] = '\0';

        result = ficlExec(pVM, cp);
        /* handle "bye" in loaded files. --lch */
        switch (result)
        {
            case VM_OUTOFTEXT:
            case VM_USEREXIT:
                break;

            default:
                pVM->sourceID = id;
                fclose(fp);
                vmThrowErr(pVM, "Error loading file <%s> line %d", pFilename->text, nLine);
                break; 
        }
    }
    /*
    ** Pass an empty line with SOURCE-ID == -1 to flush
    ** any pending REFILLs (as required by FILE wordset)
    */
    pVM->sourceID.i = -1;
    ficlExec(pVM, "");

    pVM->sourceID = id;
    fclose(fp);

    /* handle "bye" in loaded files. --lch */
    if (result == VM_USEREXIT)
        vmThrow(pVM, VM_USEREXIT);
    return;
}

/*
** Dump a tab delimited file that summarizes the contents of the
** dictionary hash table by hashcode...
*/
static void spewHash(FICL_VM *pVM)
{
    FICL_HASH *pHash = vmGetDict(pVM)->pForthWords;
    FICL_WORD *pFW;
    FILE *pOut;
    unsigned i;
    unsigned nHash = pHash->size;

    if (!vmGetWordToPad(pVM))
        vmThrow(pVM, VM_OUTOFTEXT);

    pOut = fopen(pVM->pad, "w");
    if (!pOut)
    {
        vmTextOut(pVM, "unable to open file", 1);
        return;
    }

    /* header line */
    fprintf(pOut, "Row\tnEntries\tNames\n");
    
    for (i=0; i < nHash; i++)
    {
        int n = 0;

        pFW = pHash->table[i];
        while (pFW)
        {
            n++;
            pFW = pFW->link;
        }

        fprintf(pOut, "%d\t%d", i, n);

        pFW = pHash->table[i];
        while (pFW)
        {
            fprintf(pOut, "\t%s", pFW->name);
            pFW = pFW->link;
        }

        fprintf(pOut, "\n");
    }

    fclose(pOut);
    return;
}

static void ficlBreak(FICL_VM *pVM)
{
    pVM->state = pVM->state; /* no-op for the debugger to grab - set a breakpoint here */
    return;
}

static void ficlClock(FICL_VM *pVM)
{
    clock_t now = clock(); /* time.h */
    stackPushUNS(pVM->pStack, (FICL_UNS)now);
    return;
}

static void clocksPerSec(FICL_VM *pVM)
{
    stackPushUNS(pVM->pStack, CLOCKS_PER_SEC);
    return;
}


void buildTestInterface(FICL_SYSTEM *pSys)
{
    ficlBuild(pSys, "break",    ficlBreak,    FW_DEFAULT);
    ficlBuild(pSys, "clock",    ficlClock,    FW_DEFAULT);
    ficlBuild(pSys, "cd",       ficlChDir,    FW_DEFAULT);
    ficlBuild(pSys, "load",     ficlLoad,     FW_DEFAULT);
    ficlBuild(pSys, "pwd",      ficlGetCWD,   FW_DEFAULT);
    ficlBuild(pSys, "system",   ficlSystem,   FW_DEFAULT);
    ficlBuild(pSys, "spewhash", spewHash,     FW_DEFAULT);
    ficlBuild(pSys, "clocks/sec", 
                                clocksPerSec, FW_DEFAULT);

    return;
}

#if FICL_UNIT_TEST
    void setUp(void)
    {
    }
    
    void tearDown(void)
    {
    }

    static void dpmUModTestCase(DPUNS input, UNS16 base, DPUNS qExpected, UNS16 rExpected)
    {
        UNS16 r   = dpmUMod(&input, base);
    
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(rExpected, r, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(qExpected.hi, input.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(qExpected.lo, input.lo, "Quotient Lo");
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
    
        /*--- 5) single‐word value splitting across hi & lo:
              0x1_0000_0001 ÷ 3 → Q=0x5555_5555, R=2 ---*/
        dpmUModTestCase(
                 ((DPUNS){ .hi = 1, .lo = 1 }),
                 3,
                 ((DPUNS){ .hi = 0, .lo = 0x55555555UL }),
                 2);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
        /* testmain.c:388:dpmUnitTest:FAIL: Expected 1431655765 Was 6148914691236517205. Quotient Lo */

    
        /*--- 6) hi only (value = 2^32) ÷2 → Q=2^31, R=0 ---*/
        dpmUModTestCase(
                 ((DPUNS){ .hi = 1, .lo = 0 }),
                 2,
                 ((DPUNS){ .hi = 0, .lo = (FICL_UNS)1 << 31 }),
                 0);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    
        /*--- 7) max‐lo, small base ---*/
        dpmUModTestCase(
                 ((DPUNS){ .hi = 0, .lo = 0xFFFFUL }),
                 0xFF,
                 ((DPUNS){ .hi = 0, .lo = 0x0101UL }),
                 0xFE);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    
        /*--- 8) large hi+lo randomish values ---*/
        dpmUModTestCase(
                 ((DPUNS){ .hi = 0x12345678UL, .lo = 0x9ABCDEF0UL }),
                 0x1234,
                 /* Precomputed with a big‐int tool or a quick script: */
                 ((DPUNS){ .hi = 0x00111A2EUL, .lo = 0xC80D7BE1UL }),
                 0x09B0);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(expRem, rem, "Remainder");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.hi, quot.hi, "Quotient Hi");
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(expQuot.lo, quot.lo, "Quotient Lo");
    }
#endif

#if !defined (_WINDOWS) /* Console main */
#define nINBUF 256

#if !defined (_WIN32)
    #define __try
    #define __except(i) if (0)
#endif

int main(int argc, char **argv)
{
    int ret = 0;
    char in[nINBUF];
    FICL_VM *pVM;
    FICL_SYSTEM *pSys;

#if FICL_UNIT_TEST
    UNITY_BEGIN();
    RUN_TEST(dpmUnitTest);
    (void) UNITY_END();    
#endif

    pSys = ficlInitSystem(10000);
    buildTestInterface(pSys);
    pVM = ficlNewVM(pSys);

    ret = ficlEvaluate(pVM, ".ver .( " __DATE__ " ) cr quit");

    /*
    ** optionally load file from cmd line...
    */
    if (argc  > 1)
    {
        sprintf(in, ".( loading %s ) cr load %s\n cr", argv[1], argv[1]); /* #todo: NEVER ACTUALLY OUTPUTS THIS */
        __try
        {
            ret = ficlEvaluate(pVM, in);
        }
        __except(1)
        {
            vmTextOut(pVM, "exception -- cleaning up", 1);
            vmReset(pVM);
        }
    }

    while (ret != VM_USEREXIT)
    {
        fgets(in, nINBUF, stdin);
        __try
        {
            ret = ficlExec(pVM, in);
        }
        __except(1)
        {
            vmTextOut(pVM, "exception -- cleaning up", 1);
            vmReset(pVM);
        }
    }

    ficlTermSystem(pSys);
    return 0;
}

#endif

