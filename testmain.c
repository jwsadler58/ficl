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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <direct.h>
#define getcwd _getcwd
#define chdir  _chdir
#else
#include <unistd.h>
#endif

#include "ficl.h"

#if FICL_UNIT_TEST
    #include "dpmath.h"
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
            vmTextOut(pVM, "Error: path not found", 0);
            vmTextOut(pVM, pFS->text, 1);
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
** Load a text file and execute it
** TODO: add correct behavior for nested loads
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
        vmTextOut(pVM, "Warning (load): empty filename", 1);
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

    vmTextOut(pVM, "Loading: ", 0); 
    vmTextOut(pVM, pFilename->text, 1);

    /* save any prior file in process*/
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

/* 
**             t e s t - e r r o r
** Test error reporting callback for scripted tests (see test/tester.fr)
*/
static int nTestFails = 0;
static void testError(FICL_VM *pVM)
{
    nTestFails++; 
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
    ficlBuild(pSys, "test-error", testError,  FW_DEFAULT); /* signaling from ficltest.fr */
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

    static void dummyCode(FICL_VM *pVM)
    {
        (void)pVM;
    }

    static void wordLayoutTest(void)
    {
        TEST_ASSERT_TRUE((FICL_WORD_HEADER_BYTES % sizeof(CELL)) == 0);
        TEST_ASSERT_TRUE(FICL_WORD_BASE_BYTES == FICL_WORD_HEADER_BYTES + sizeof(CELL));
        TEST_ASSERT_TRUE(FICL_WORD_BASE_CELLS == (FICL_WORD_BASE_BYTES / sizeof(CELL)));
    }

    static void wordAppendBodyTest(void)
    {
        FICL_DICT *dp = dictCreate(256);
        STRINGINFO si;
        FICL_WORD *pFW;
        const char *name = "testword";

        SI_SETLEN(si, strlen(name));
        SI_SETPTR(si, (char *)name);
        pFW = dictAppendWord2(dp, si, dummyCode, FW_DEFAULT);

        TEST_ASSERT_TRUE(pFW != NULL);
        TEST_ASSERT_TRUE(pFW->param == dp->here);
        TEST_ASSERT_TRUE(pFW->nName == (FICL_COUNT)strlen(name));
        {
            CELL *body = pFW->param + 1;
            FICL_WORD *from = (FICL_WORD *)((char *)body - FICL_WORD_BASE_BYTES);
            TEST_ASSERT_TRUE(from == pFW);
        }

        dictDelete(dp);
    }

    static void hashLayoutTest(void)
    {
        size_t base = offsetof(FICL_HASH, table);
        TEST_ASSERT_TRUE((base % sizeof(void *)) == 0);
        TEST_ASSERT_TRUE(FICL_HASH_BYTES(1) == base + sizeof(FICL_WORD *));
        TEST_ASSERT_TRUE(FICL_HASH_BYTES(4) == base + 4 * sizeof(FICL_WORD *));
    }

    static void hashCreateTest(void)
    {
        FICL_DICT *dp = dictCreateHashed(256, 7);
        FICL_HASH *pHash = dp->pForthWords;
        unsigned i;

        TEST_ASSERT_TRUE(pHash != NULL);
        TEST_ASSERT_TRUE(pHash->size == 7);
        for (i = 0; i < pHash->size; ++i)
        {
            TEST_ASSERT_TRUE(pHash->table[i] == NULL);
        }

        dictDelete(dp);
    }

#endif

#define nINBUF 256

int main(int argc, char **argv)
{
    int ret = 0;
    char in[nINBUF];
    FICL_VM *pVM;
    FICL_SYSTEM *pSys;
    int i;
    int fUnit = 0;

    /* Find '--test' anywhere on the command line */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--test") == 0) 
        {
#if FICL_UNIT_TEST
            fUnit = 1;
            break;
#else
            printf("Error: Unit tests not enabled in this build.\n");
            return 1; 
#endif
        }
    }

#if FICL_UNIT_TEST
    if (fUnit)
    {
        UNITY_BEGIN();
        RUN_TEST(dpmUnitTest);
        RUN_TEST(wordLayoutTest);
        RUN_TEST(wordAppendBodyTest);
        RUN_TEST(hashLayoutTest);
        RUN_TEST(hashCreateTest);
        nTestFails = UNITY_END(); 
        if (fUnit && (nTestFails > 0))
        {
            printf("Unit tests failed: %d\n", nTestFails);
            return nTestFails;
        }
    }
#endif

    pSys = ficlInitSystem(20000);
    buildTestInterface(pSys);
    pVM = ficlNewVM(pSys);

    ret = ficlEvaluate(pVM, ".ver 2 spaces .( " __DATE__ " ) cr");
    if (fUnit)                 /* run tests and return rather than going interactive */
    {
        ret = ficlEvaluate(pVM, "cd test\n load ficltest.fr");
        ficlTermSystem(pSys);
        printf("***** Scripted tests completed: %d failures *****\n", nTestFails);
        return nTestFails;
    }

    while (ret != VM_USEREXIT)
    {
        (void) fgets(in, nINBUF, stdin);
        ret = ficlExec(pVM, in);
    }

    ficlTermSystem(pSys);
    return 0;
}


