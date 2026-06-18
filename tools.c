/*******************************************************************
** t o o l s . c
** Forth Inspired Command Language - programming tools
** Author: John W Sadler
** Created: 20 June 2000
** $Id: tools.c,v 1.11 2001-12-04 17:58:14-08 jsadler Exp jsadler $
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

/*
** NOTES:
** SEE needs information about the addresses of functions that
** are the CFAs of colon definitions, constants, variables, DOES>
** words, and so on. It gets this information from ficlWordClassify()
** in words.c, which classifies words by their opcode field.
**
** Step and break debugger for Ficl
** debug  ( xt -- )   Start debugging an xt
** Set a breakpoint
** Specify breakpoint default action
*/

#include <stdlib.h>
#include <stdio.h>          /* snprintf */
#include <string.h>
#include "ficl.h"



/**************************************************************************
                        v m S e t B r e a k
** Set a breakpoint at the current value of IP by
** storing that address in a BREAKPOINT record
**************************************************************************/
static void vmSetBreak(FICL_VM *pVM, FICL_BREAKPOINT *pBP)
{
    pBP->address      = pVM->ip;
    pBP->origCell     = *pVM->ip;
    pVM->ip->u        = (FICL_UNS)FICL_OP_STEP_BREAK;
}


/**************************************************************************
**                      d e b u g P r o m p t
**************************************************************************/
static void debugPrompt(FICL_VM *pVM)
{
    vmTextOut(pVM, "dbg> ", false);
}


/**************************************************************************
**                      i s A F i c l W o r d
** Vet a candidate pointer carefully to make sure
** it's not some chunk o' inline data...
** It has to have a name, and it has to look
** like it's in the dictionary address range.
** NOTE: this excludes :noname words!
**************************************************************************/
bool isAFiclWord(FICL_DICT *pd, FICL_WORD *pFW)
{

    if (!dictIncludes(pd, pFW))
        return false;

    if (!dictIncludes(pd, pFW->name))
        return false;

    if ((pFW->link != NULL) && !dictIncludes(pd, pFW->link))
        return false;

    if ((pFW->nName <= 0) || (pFW->name[pFW->nName] != '\0'))
        return false;

    if (strlen(pFW->name) != pFW->nName)
        return false;

    return true;
}


#if 0
static int isPrimitive(FICL_WORD *pFW)
{
    WORDKIND wk = ficlWordClassify(pFW);
    return (wk != COLON) && (wk != DOES);
}
#endif


/**************************************************************************
                        f i n d E n c l o s i n g W o r d
** Given a pointer to something, check to make sure it's an address in the
** dictionary. If so, search backwards until we find something that looks
** like a dictionary header. If successful, return the address of the
** FICL_WORD found. Otherwise return NULL.
** nSEARCH_CELLS sets the maximum neighborhood this func will search before giving up
**************************************************************************/
#define nSEARCH_CELLS 100

static FICL_WORD *findEnclosingWord(FICL_VM *pVM, CELL *cp)
{
    FICL_WORD *pFW;
    FICL_DICT *pd = vmGetDict(pVM);
    int i;

    if (!dictIncludes(pd, (void *)cp))
        return NULL;

    for (i = nSEARCH_CELLS; i > 0; --i, --cp)
    {
        pFW = (FICL_WORD *)(cp + 1 - FICL_WORD_BASE_CELLS);
        if (isAFiclWord(pd, pFW))
            return pFW;
    }

    return NULL;
}


/**************************************************************************
                        s e e
** TOOLS ( "<spaces>name" -- )
** Display a human-readable representation of the named word's definition.
** The source of the representation (object-code decompilation, source
** block, etc.) and the particular form of the display is implementation
** defined.
**************************************************************************/
/*
** Opcode format tags — how seeColon should print and advance past each opcode.
*/
typedef enum {
    BC_FMT_PLAIN,       /* no extra data                                    */
    BC_FMT_IMM,         /* show signed immediate from upper 56 bits         */
    BC_FMT_BRANCH,      /* imm = branch offset relative to next cell        */
    BC_FMT_COLON,       /* imm = dict offset; colon-call target             */
    BC_FMT_LEAVE,       /* next cell = absolute CELL* leave target          */
    BC_FMT_WORDPTR,     /* next cell = FICL_WORD*, show name                */
    BC_FMT_DOESPTR,     /* next cell = FICL_WORD*, show name (DOES>)        */
    BC_FMT_WIDLIT,      /* next cell = full CELL literal value              */
    BC_FMT_2LIT,        /* next 2 cells = double-cell literal               */
    BC_FMT_STRINGLIT,   /* next bytes = FICL_STRING (variable length)       */
    BC_FMT_CSTRINGLIT,  /* same, but c-string form                          */
    BC_FMT_FLIT,        /* next cell = FICL_FLOAT                           */
} BC_FMT;

typedef struct { const char *name; BC_FMT fmt; } BC_ATTR_ENTRY;

static const BC_ATTR_ENTRY bcAttrTable[FICL_OP_COUNT] = {
    [FICL_OP_CALL]          = { "call",     BC_FMT_WORDPTR   },
    [FICL_OP_DUP]           = { "dup",      BC_FMT_PLAIN     },
    [FICL_OP_DROP]          = { "drop",     BC_FMT_PLAIN     },
    [FICL_OP_SWAP]          = { "swap",     BC_FMT_PLAIN     },
    [FICL_OP_OVER]          = { "over",     BC_FMT_PLAIN     },
    [FICL_OP_ROT]           = { "rot",      BC_FMT_PLAIN     },
    [FICL_OP_MINUS_ROT]     = { "-rot",     BC_FMT_PLAIN     },
    [FICL_OP_PICK]          = { "pick",     BC_FMT_PLAIN     },
    [FICL_OP_ROLL]          = { "roll",     BC_FMT_PLAIN     },
    [FICL_OP_MINUS_ROLL]    = { "-roll",    BC_FMT_PLAIN     },
    [FICL_OP_2DUP]          = { "2dup",     BC_FMT_PLAIN     },
    [FICL_OP_2DROP]         = { "2drop",    BC_FMT_PLAIN     },
    [FICL_OP_2SWAP]         = { "2swap",    BC_FMT_PLAIN     },
    [FICL_OP_2OVER]         = { "2over",    BC_FMT_PLAIN     },
    [FICL_OP_QUESTION_DUP]  = { "?dup",     BC_FMT_PLAIN     },
    [FICL_OP_FETCH]         = { "@",        BC_FMT_PLAIN     },
    [FICL_OP_STORE]         = { "!",        BC_FMT_PLAIN     },
    [FICL_OP_2FETCH]        = { "2@",       BC_FMT_PLAIN     },
    [FICL_OP_2STORE]        = { "2!",       BC_FMT_PLAIN     },
    [FICL_OP_PLUS_STORE]    = { "+!",       BC_FMT_PLAIN     },
    [FICL_OP_C_FETCH]       = { "c@",       BC_FMT_PLAIN     },
    [FICL_OP_C_STORE]       = { "c!",       BC_FMT_PLAIN     },
    [FICL_OP_W_FETCH]       = { "w@",       BC_FMT_PLAIN     },
    [FICL_OP_W_STORE]       = { "w!",       BC_FMT_PLAIN     },
    [FICL_OP_PLUS]          = { "+",        BC_FMT_PLAIN     },
    [FICL_OP_MINUS]         = { "-",        BC_FMT_PLAIN     },
    [FICL_OP_STAR]          = { "*",        BC_FMT_PLAIN     },
    [FICL_OP_SLASH]         = { "/",        BC_FMT_PLAIN     },
    [FICL_OP_MOD]           = { "mod",      BC_FMT_PLAIN     },
    [FICL_OP_SLASH_MOD]     = { "/mod",     BC_FMT_PLAIN     },
    [FICL_OP_STAR_SLASH]    = { "*/",       BC_FMT_PLAIN     },
    [FICL_OP_STAR_SLASH_MOD]= { "*/mod",    BC_FMT_PLAIN     },
    [FICL_OP_ONE_PLUS]      = { "1+",       BC_FMT_PLAIN     },
    [FICL_OP_ONE_MINUS]     = { "1-",       BC_FMT_PLAIN     },
    [FICL_OP_TWO_STAR]      = { "2*",       BC_FMT_PLAIN     },
    [FICL_OP_TWO_SLASH]     = { "2/",       BC_FMT_PLAIN     },
    [FICL_OP_NEGATE]        = { "negate",   BC_FMT_PLAIN     },
    [FICL_OP_MAX]           = { "max",      BC_FMT_PLAIN     },
    [FICL_OP_MIN]           = { "min",      BC_FMT_PLAIN     },
    [FICL_OP_ZERO_LESS]     = { "0<",       BC_FMT_PLAIN     },
    [FICL_OP_ZERO_EQUALS]   = { "0=",       BC_FMT_PLAIN     },
    [FICL_OP_ZERO_GREATER]  = { "0>",       BC_FMT_PLAIN     },
    [FICL_OP_LESS]          = { "<",        BC_FMT_PLAIN     },
    [FICL_OP_EQUALS]        = { "=",        BC_FMT_PLAIN     },
    [FICL_OP_GREATER]       = { ">",        BC_FMT_PLAIN     },
    [FICL_OP_U_LESS]        = { "u<",       BC_FMT_PLAIN     },
    [FICL_OP_AND]           = { "and",      BC_FMT_PLAIN     },
    [FICL_OP_OR]            = { "or",       BC_FMT_PLAIN     },
    [FICL_OP_XOR]           = { "xor",      BC_FMT_PLAIN     },
    [FICL_OP_INVERT]        = { "invert",   BC_FMT_PLAIN     },
    [FICL_OP_LSHIFT]        = { "lshift",   BC_FMT_PLAIN     },
    [FICL_OP_RSHIFT]        = { "rshift",   BC_FMT_PLAIN     },
    [FICL_OP_TO_R]          = { ">r",       BC_FMT_PLAIN     },
    [FICL_OP_R_FROM]        = { "r>",       BC_FMT_PLAIN     },
    [FICL_OP_R_FETCH]       = { "r@",       BC_FMT_PLAIN     },
    [FICL_OP_2TO_R]         = { "2>r",      BC_FMT_PLAIN     },
    [FICL_OP_2R_FROM]       = { "2r>",      BC_FMT_PLAIN     },
    [FICL_OP_2R_FETCH]      = { "2r@",      BC_FMT_PLAIN     },
    [FICL_OP_DEPTH]         = { "depth",    BC_FMT_PLAIN     },
    [FICL_OP_BRANCH]        = { "branch",   BC_FMT_BRANCH    },
    [FICL_OP_BRANCH0]       = { "branch0",  BC_FMT_BRANCH    },
    [FICL_OP_DO]            = { "do",       BC_FMT_LEAVE     },
    [FICL_OP_QDO]           = { "?do",      BC_FMT_LEAVE     },
    [FICL_OP_LOOP]          = { "loop",     BC_FMT_BRANCH    },
    [FICL_OP_PLOOP]         = { "+loop",    BC_FMT_BRANCH    },
    [FICL_OP_LIT]           = { "lit",      BC_FMT_IMM       },
    [FICL_OP_2LIT]          = { "2lit",     BC_FMT_2LIT      },
    [FICL_OP_EXIT]          = { "exit",     BC_FMT_PLAIN     },
    [FICL_OP_SEMI]          = { ";",        BC_FMT_PLAIN     },
    [FICL_OP_OF]            = { "of",       BC_FMT_BRANCH    },
    [FICL_OP_LEAVE]         = { "leave",    BC_FMT_PLAIN     },
    [FICL_OP_UNLOOP]        = { "unloop",   BC_FMT_PLAIN     },
    [FICL_OP_COLON]         = { "colon",    BC_FMT_COLON     },
    [FICL_OP_CONSTANT]      = { "constant", BC_FMT_PLAIN     },
    [FICL_OP_2CONSTANT]     = { "2constant",BC_FMT_PLAIN     },
    [FICL_OP_VALUE]         = { "value",   BC_FMT_IMM       },
    [FICL_OP_CREATE]        = { "create",   BC_FMT_PLAIN     },
    [FICL_OP_DOES]          = { "does>",    BC_FMT_PLAIN     },
    [FICL_OP_VARIABLE]      = { "variable", BC_FMT_PLAIN     },
    [FICL_OP_STRINGLIT]     = { "s\"",      BC_FMT_STRINGLIT },
    [FICL_OP_CSTRINGLIT]    = { "c\"",      BC_FMT_CSTRINGLIT},
    [FICL_OP_ADDRLIT]       = { "addrlit",  BC_FMT_IMM       },
    [FICL_OP_WIDLIT]        = { "widlit",   BC_FMT_WIDLIT    },
    [FICL_OP_DOES_CALL]     = { "does-call",BC_FMT_DOESPTR   },
    [FICL_OP_STEP_BREAK]    = { "step-break",BC_FMT_PLAIN    },
#if FICL_WANT_USER
    [FICL_OP_USER]          = { "user",     BC_FMT_IMM       },
#endif
#if FICL_WANT_FLOAT
    [FICL_OP_FCONSTANT]     = { "fconstant",BC_FMT_PLAIN     },
    [FICL_OP_FDUP]          = { "fdup",     BC_FMT_PLAIN     },
    [FICL_OP_FDROP]         = { "fdrop",    BC_FMT_PLAIN     },
    [FICL_OP_FSWAP]         = { "fswap",    BC_FMT_PLAIN     },
    [FICL_OP_FOVER]         = { "fover",    BC_FMT_PLAIN     },
    [FICL_OP_FROT]          = { "frot",     BC_FMT_PLAIN     },
    [FICL_OP_FMINUS_ROT]    = { "f-rot",    BC_FMT_PLAIN     },
    [FICL_OP_FPICK]         = { "fpick",    BC_FMT_PLAIN     },
    [FICL_OP_FROLL]         = { "froll",    BC_FMT_PLAIN     },
    [FICL_OP_FMINUS_ROLL]   = { "f-roll",   BC_FMT_PLAIN     },
    [FICL_OP_F2DUP]         = { "f2dup",    BC_FMT_PLAIN     },
    [FICL_OP_F2DROP]        = { "f2drop",   BC_FMT_PLAIN     },
    [FICL_OP_F2SWAP]        = { "f2swap",   BC_FMT_PLAIN     },
    [FICL_OP_F2OVER]        = { "f2over",   BC_FMT_PLAIN     },
    [FICL_OP_FQUESTION_DUP] = { "f?dup",    BC_FMT_PLAIN     },
    [FICL_OP_FPLUS]         = { "f+",       BC_FMT_PLAIN     },
    [FICL_OP_FMINUS]        = { "f-",       BC_FMT_PLAIN     },
    [FICL_OP_FSTAR]         = { "f*",       BC_FMT_PLAIN     },
    [FICL_OP_FSLASH]        = { "f/",       BC_FMT_PLAIN     },
    [FICL_OP_FNEGATE]       = { "fnegate",  BC_FMT_PLAIN     },
    [FICL_OP_FABS]          = { "fabs",     BC_FMT_PLAIN     },
    [FICL_OP_FMAX]          = { "fmax",     BC_FMT_PLAIN     },
    [FICL_OP_FMIN]          = { "fmin",     BC_FMT_PLAIN     },
    [FICL_OP_FPLUS_STORE]   = { "f+!",      BC_FMT_PLAIN     },
    [FICL_OP_FFETCH]        = { "f@",       BC_FMT_PLAIN     },
    [FICL_OP_FSTORE]        = { "f!",       BC_FMT_PLAIN     },
    [FICL_OP_F0LESS]        = { "f0<",      BC_FMT_PLAIN     },
    [FICL_OP_F0EQUALS]      = { "f0=",      BC_FMT_PLAIN     },
    [FICL_OP_F0GREATER]     = { "f0>",      BC_FMT_PLAIN     },
    [FICL_OP_FLESS]         = { "f<",       BC_FMT_PLAIN     },
    [FICL_OP_FCLOSE]        = { "f~",       BC_FMT_PLAIN     },
    [FICL_OP_FEQUAL]        = { "f=",       BC_FMT_PLAIN     },
    [FICL_OP_FGREATER]      = { "f>",       BC_FMT_PLAIN     },
    [FICL_OP_FDEPTH]        = { "fdepth",   BC_FMT_PLAIN     },
    [FICL_OP_S_TO_F]        = { "s>f",      BC_FMT_PLAIN     },
    [FICL_OP_F_TO_S]        = { "f>s",      BC_FMT_PLAIN     },
    [FICL_OP_FPLUS_I]       = { "f+i",      BC_FMT_PLAIN     },
    [FICL_OP_FMINUS_I]      = { "f-i",      BC_FMT_PLAIN     },
    [FICL_OP_FSTAR_I]       = { "f*i",      BC_FMT_PLAIN     },
    [FICL_OP_FSLASH_I]      = { "f/i",      BC_FMT_PLAIN     },
    [FICL_OP_I_MINUS_F]     = { "i-f",      BC_FMT_PLAIN     },
    [FICL_OP_I_SLASH_F]     = { "i/f",      BC_FMT_PLAIN     },
    [FICL_OP_FLIT]          = { "flit",     BC_FMT_FLIT      },
#endif
};

/*
** Return the attribute table entry for an opcode, substituting a
** sentinel for any unrecognised value.
*/
static const BC_ATTR_ENTRY *bcAttr(FICL_OPCODE op)
{
    static const BC_ATTR_ENTRY unknown = { "?op", BC_FMT_PLAIN };
    if ((unsigned)op >= FICL_OP_COUNT || bcAttrTable[op].name == NULL)
        return &unknown;
    return &bcAttrTable[op];
}


/*
** seeColon -- walks a colon body and prints a disassembly listing.
** Uses the BC_ATTR table to decode each bytecode cell.
*/
static void seeColon(FICL_VM *pVM, CELL *pc)
{
    char outbuf[nPAD];
    CELL *param0 = pc;

    #define SNLIMIT (sizeof(outbuf) - (size_t)(cp - outbuf))

    for (;;)
    {
        CELL *pc0    = pc;                      /* start of this instruction */
        FICL_OPCODE op  = BC_OP(*pc);
        FICL_INT    imm = BC_IMM(*pc);
        const BC_ATTR_ENTRY *a = bcAttr(op);
        char *cp = outbuf;

        pc++;                                   /* advance past opcode cell  */

        *cp++ = (pc0 == pVM->ip) ? '>' : ' ';
        cp += snprintf(cp, SNLIMIT, "%3d  %-10s", (int)(pc0 - param0), a->name);

        switch (a->fmt)
        {
        case BC_FMT_IMM:
            cp += snprintf(cp, SNLIMIT, PCT_LD, imm);
            break;

        case BC_FMT_BRANCH:
            cp += snprintf(cp, SNLIMIT, "->%d", (int)(pc - param0) + (int)imm);
            break;

        case BC_FMT_COLON:
            {
                /* imm is a dict-relative cell offset; try to find the word */
                CELL *target = pVM->pSys->dp->dict + imm;
                FICL_WORD *pw = findEnclosingWord(pVM, target);
                if (pw)
                    cp += snprintf(cp, SNLIMIT, "%.*s", pw->nName, pw->name);
                else
                    cp += snprintf(cp, SNLIMIT, "dict+%d", (int)imm);
            }
            break;

        case BC_FMT_LEAVE:
            {
                CELL *target = (CELL *)(pc++)->p;
                cp += snprintf(cp, SNLIMIT, "leave->%d", (int)(target - param0));
            }
            break;

        case BC_FMT_WORDPTR:
        case BC_FMT_DOESPTR:
            {
                FICL_WORD *pw = (FICL_WORD *)(pc++)->p;
                cp += snprintf(cp, SNLIMIT, "%.*s", pw->nName, pw->name);
            }
            break;

        case BC_FMT_WIDLIT:
            cp += snprintf(cp, SNLIMIT, PCT_LD " (0x" PCT_IX ")", pc->i, pc->u);
            pc++;
            break;

        case BC_FMT_2LIT:
            cp += snprintf(cp, SNLIMIT, PCT_LD " " PCT_LD, pc[0].i, pc[1].i);
            pc += 2;
            break;

        case BC_FMT_STRINGLIT:
        case BC_FMT_CSTRINGLIT:
            {
                FICL_STRING *sp = (FICL_STRING *)pc;
                cp += snprintf(cp, SNLIMIT, "\"%.*s\"", sp->count, sp->text);
                pc = (CELL *)alignPtr(sp->text + sp->count + 1);
            }
            break;

        case BC_FMT_FLIT:
            {
#if FICL_WANT_FLOAT
                FICL_FLOAT f;
                memcpy(&f, pc, sizeof(f));
                cp += snprintf(cp, SNLIMIT, "%g", (double)f);
                pc++;
#endif
            }
            break;

        default:
            break;
        }

        vmTextOut(pVM, outbuf, true);

        if (op == FICL_OP_SEMI)
            break;
    }

    #undef SNLIMIT
}

/*
** Here's the outer part of the decompiler. It's
** just a big nested conditional that checks the
** CFA of the word to decompile for each kind of
** known word-builder code, and tries to do
** something appropriate. If the CFA is not recognized,
** just indicate that it is a primitive.
*/
static void seeXT(FICL_VM *pVM)
{
    FICL_WORD *pFW;
    WORDKIND kind;
    char outbuf[nPAD];

    pFW = (FICL_WORD *)stackPopPtr(pVM->pStack);
    kind = ficlWordClassify(pFW);

    switch (kind)
    {
    case COLON:
        snprintf(outbuf, sizeof(outbuf), ": %.*s", pFW->nName, pFW->name);
        vmTextOut(pVM, outbuf, true);
        seeColon(pVM, pFW->param);
        break;

    case DOES:
        vmTextOut(pVM, "does>", true);
        seeColon(pVM, (CELL *)pFW->param->p);
        break;

    case CREATE:
        vmTextOut(pVM, "create", true);
        break;

    case VARIABLE:
        snprintf(outbuf, sizeof(outbuf), "variable = " PCT_LD " (" PCT_IX ")",
            pFW->param->i, pFW->param->u);
        vmTextOut(pVM, outbuf, true);
        break;

#if FICL_WANT_USER
    case USER:
        snprintf(outbuf, sizeof(outbuf), "user variable " PCT_LD " (" PCT_IX ")",
            pFW->param->i, pFW->param->u);
        vmTextOut(pVM, outbuf, true);
        break;
#endif

    case CONSTANT:
        snprintf(outbuf, sizeof(outbuf), "constant = " PCT_LD " (" PCT_IX ")",
            pFW->param->i, pFW->param->u);
        vmTextOut(pVM, outbuf, true);
        break;

    case VALUE:
        snprintf(outbuf, sizeof(outbuf), "value = " PCT_LD " (" PCT_IX ")",
            pFW->param->i, pFW->param->u);
        vmTextOut(pVM, outbuf, true);
        break;

    default:
        snprintf(outbuf, sizeof(outbuf), "%.*s is a primitive", pFW->nName, pFW->name);
        vmTextOut(pVM, outbuf, true);
        break;
    }

    if (pFW->flags & FW_IMMEDIATE)
    {
        vmTextOut(pVM, "immediate", true);
    }

    if (pFW->flags & FW_COMPILE)
    {
        vmTextOut(pVM, "compile-only", true);
    }

    return;
}


static void see(FICL_VM *pVM)
{
    ficlTick(pVM);
    seeXT(pVM);
    return;
}


/**************************************************************************
                        f i c l D e b u g X T
** debug  ( xt -- )
** Given an xt of a colon definition or a word defined by DOES>, set the
** VM up to debug the word: push IP, set the xt as the next thing to execute,
** set a breakpoint at its first instruction, and run to the breakpoint.
** Note: the semantics of this word are equivalent to "step in"
**************************************************************************/
static void ficlDebugXT(FICL_VM *pVM)
{
    FICL_WORD *xt    = (FICL_WORD *)stackPopPtr(pVM->pStack);
    WORDKIND   wk    = ficlWordClassify(xt);

    stackPushPtr(pVM->pStack, xt);
    seeXT(pVM);

    switch (wk)
    {
    case COLON:
    case DOES:
        /*
        ** Run the colon code and set a breakpoint at the next instruction
        */
        vmExecute(pVM, xt);
        vmSetBreak(pVM, &(pVM->pSys->bpStep));
        break;

    default:
        vmExecute(pVM, xt);
        break;
    }

    return;
}


/**************************************************************************
                        s t e p I n
** FICL
** Execute the next instruction, stepping into it if it's a colon definition
** or a does> word. This is the easy kind of step.
**************************************************************************/
static void stepIn(FICL_VM *pVM)
{
    /*
    ** Do one step of the inner loop
    */
    {
        vmStep(pVM);
    }

    /*
    ** Now set a breakpoint at the next instruction
    */
    vmSetBreak(pVM, &(pVM->pSys->bpStep));

    return;
}


/**************************************************************************
                        s t e p O v e r
** FICL
** Execute the next instruction atomically. This requires some insight into
** the memory layout of compiled code. Set a breakpoint at the next instruction
** in this word, and run until we hit it
**************************************************************************/
static void stepOver(FICL_VM *pVM)
{
    FICL_OPCODE op   = BC_OP(*pVM->ip);
    FICL_WORD  *pFW  = NULL;
    WORDKIND    kind = PRIMITIVE;
    CELL       *next;

    switch (op)
    {
    case FICL_OP_CALL:
    case FICL_OP_DOES_CALL:
        pFW  = (FICL_WORD *)(pVM->ip[1].p);
        kind = ficlWordClassify(pFW);
        next = pVM->ip + 2;
        break;
    case FICL_OP_WIDLIT:
    case FICL_OP_DO:
    case FICL_OP_QDO:
        next = pVM->ip + 2;
        break;
    case FICL_OP_2LIT:
        next = pVM->ip + 3;
        break;
    default:
        next = pVM->ip + 1;
        break;
    }

    if (kind == COLON || kind == DOES)
    {
        /* Let the colon word run to completion; break at the next instruction. */
        pVM->pSys->bpStep.address  = next;
        pVM->pSys->bpStep.origCell = *next;
        next->u = (FICL_UNS)FICL_OP_STEP_BREAK;
    }
    else
    {
        stepIn(pVM);
    }

    return;
}


/**************************************************************************
                        s t e p - b r e a k
** FICL
** Handles breakpoints for stepped execution.
** Upon entry, bpStep contains the address and replaced instruction
** of the current breakpoint.
** Clear the breakpoint
** Get a command from the console.
** i (step in) - execute the current instruction and set a new breakpoint
**    at the IP
** o (step over) - execute the current instruction to completion and set
**    a new breakpoint at the IP
** g (go) - execute the current instruction and exit
** q (quit) - abort current word
** b (toggle breakpoint)
**************************************************************************/
static void stepBreak(FICL_VM *pVM)
{
    STRINGINFO si;
    FICL_WORD *pFW;
    FICL_WORD *pOnStep;
    char outbuf[nPAD];

    if (!pVM->fRestart)
    {
        assert(pVM->pSys->bpStep.address);
        /*
        ** Restore the original instruction cell and rewind IP so the
        ** restored instruction executes next.
        */
        *(CELL *)(pVM->pSys->bpStep.address) = pVM->pSys->bpStep.origCell;
        pVM->ip = (IPTYPE)(pVM->pSys->bpStep.address);

        /*
        ** If there's an onStep, do it
        */
        pOnStep = ficlLookup(pVM->pSys, "on-step");
        if (pOnStep)
            ficlExecXT(pVM, pOnStep);

        /*
        ** Print the name of the next instruction.  In bytecode mode the cell
        ** at the breakpoint address is the opcode cell; if it is FICL_OP_CALL
        ** the word pointer lives in the following cell.
        */
        pFW = ((BC_OP(pVM->pSys->bpStep.origCell) == FICL_OP_CALL))
              ? (FICL_WORD *)(pVM->ip[1].p) : NULL;
        if (pFW)
            snprintf(outbuf, sizeof(outbuf), "next: %.*s", pFW->nName, pFW->name);
        else
            snprintf(outbuf, sizeof(outbuf), "next: opcode 0x%02x", BC_OP(pVM->pSys->bpStep.origCell));
#if 0
        if (isPrimitive(pFW))
        {
            strcat(outbuf, " ( primitive )");
        }
#endif

        vmTextOut(pVM, outbuf, true);
        debugPrompt(pVM);
    }
    else
    {
        pVM->fRestart = false;
    }

    si = vmGetWord(pVM);

    if      (!strincmp(si.cp, "i", si.count))
    {
        stepIn(pVM);
    }
    else if (!strincmp(si.cp, "g", si.count))
    {
        return;
    }
    else if (!strincmp(si.cp, "l", si.count))
    {
        FICL_WORD *xt;
        xt = findEnclosingWord(pVM, (CELL *)(pVM->ip));
        if (xt)
        {
            stackPushPtr(pVM->pStack, xt);
            seeXT(pVM);
        }
        else
        {
            vmTextOut(pVM, "sorry - can't do that", true);
        }
        vmThrow(pVM, VM_RESTART);
    }
    else if (!strincmp(si.cp, "o", si.count))
    {
        stepOver(pVM);
    }
    else if (!strincmp(si.cp, "q", si.count))
    {
        vmTextOut(pVM, FICL_PROMPT, false);
        vmThrow(pVM, VM_ABORT);
    }
    else if (!strincmp(si.cp, "x", si.count))
    {
        /*
        ** Take whatever's left in the TIB and feed it to a subordinate ficlExec
        */
        int ret;
        const char *cp = pVM->tib.cp + pVM->tib.index;
        int count = pVM->tib.end - cp;
        FICL_WORD *oldRun = pVM->runningWord;

        ret = ficlExecC(pVM, cp, count);

        if (ret == VM_OUTOFTEXT)
        {
            ret = VM_RESTART;
            pVM->runningWord = oldRun;
            vmTextOut(pVM, "", true);
        }

        vmThrow(pVM, ret);
    }
    else
    {
        vmTextOut(pVM, "i -- step In", true);
        vmTextOut(pVM, "o -- step Over", true);
        vmTextOut(pVM, "g -- Go (execute to completion)", true);
        vmTextOut(pVM, "l -- List source code", true);
        vmTextOut(pVM, "q -- Quit (stop debugging and abort)", true);
        vmTextOut(pVM, "x -- eXecute the rest of the line as ficl words", true);
        debugPrompt(pVM);
        vmThrow(pVM, VM_RESTART);
    }

    return;
}


/**************************************************************************
                        b y e
** TOOLS
** Signal the system to shut down - this causes ficlExec to return
** VM_USEREXIT. The rest is up to you.
**************************************************************************/
static void bye(FICL_VM *pVM)
{
    vmThrow(pVM, VM_USEREXIT);
    return;
}


/**************************************************************************
                        d i s p l a y S t a c k
** TOOLS
** Display the parameter stack (code for ".s")
**************************************************************************/
static void displayPStack(FICL_VM *pVM)
{
    FICL_STACK *pStk = pVM->pStack;
    int d = stackDepth(pStk);
    int i;
    CELL *pCell;
    char outbuf[nPAD];

    vmCheckStack(pVM, 0, 0);

    if (d == 0)
        vmTextOut(pVM, "(Empty) ", false);
    else
    {
        pCell = pStk->base;
        for (i = 0; i < d; i++)
        {
            vmTextOut(pVM, ficlLtoa((*pCell++).i, outbuf, pVM->base), false);
            vmTextOut(pVM, " ", false);
        }
    }
    return;
}


static void displayRStack(FICL_VM *pVM)
{
    FICL_STACK *pStk = pVM->rStack;
    int d = stackDepth(pStk);
    int i;
    CELL *pCell;
    FICL_DICT *dp = vmGetDict(pVM);
    char outbuf[nPAD];

    vmCheckStack(pVM, 0, 0);

    if (d == 0)
        vmTextOut(pVM, "(Stack Empty) ", false);
    else
    {
        pCell = pStk->base;
        for (i = 0; i < d; i++)
        {
            CELL c = *pCell++;
            /*
            ** Attempt to find the word that contains the
            ** stacked address (as if it is part of a colon definition).
            ** If this works, print the name of the word. Otherwise print
            ** the value as a number.
            */
            if (dictIncludes(dp, c.p))
            {
                FICL_WORD *pFW = findEnclosingWord(pVM, (CELL *)c.p);
                if (pFW)
                {
                    int offset = (CELL *)c.p - &pFW->param[0];
                    snprintf(outbuf, sizeof(outbuf), "%s+%d ", pFW->name, offset);
                    vmTextOut(pVM, outbuf, false);
                    continue;  /* no need to print the numeric value */
                }
            }
            vmTextOut(pVM, ficlLtoa(c.i, outbuf, pVM->base), false);
            vmTextOut(pVM, " ", false);
        }
    }

    return;
}


/**************************************************************************
                        f o r g e t - w i d
**
**************************************************************************/
static void forgetWid(FICL_VM *pVM)
{
    FICL_DICT *pDict = vmGetDict(pVM);
    FICL_HASH *pHash;

    pHash = (FICL_HASH *)stackPopPtr(pVM->pStack);
    hashForget(pHash, pDict->here);

    return;
}


/**************************************************************************
                        f o r g e t
** TOOLS EXT  ( "<spaces>name" -- )
** Skip leading space delimiters. Parse name delimited by a space.
** Find name, then delete name from the dictionary along with all
** words added to the dictionary after name. An ambiguous
** condition exists if name cannot be found.
**
** If the Search-Order word set is present, FORGET searches the
** compilation word list. An ambiguous condition exists if the
** compilation word list is deleted.
**************************************************************************/
static void forget(FICL_VM *pVM)
{
    void *where;
    FICL_DICT *pDict = vmGetDict(pVM);
    FICL_HASH *pHash = pDict->pCompile;

    ficlTick(pVM);
    where = ((FICL_WORD *)stackPopPtr(pVM->pStack))->name;
    hashForget(pHash, where);
    pDict->here = PTRtoCELL where;

    return;
}


/**************************************************************************
                        l i s t W o r d s
**
**************************************************************************/
#define nCOLWIDTH 8
static void listWords(FICL_VM *pVM)
{
    FICL_DICT *dp = vmGetDict(pVM);
    FICL_HASH *pHash = dp->pSearch[dp->nLists - 1];
    FICL_WORD *wp;
    int nChars = 0;
    int len;
    unsigned i;
    int nWords = 0;
    char *cp;
    char *pScr = pVM->scratch;

    for (i = 0; i < pHash->size; i++)
    {
        for (wp = pHash->table[i]; wp != NULL; wp = wp->link, nWords++)
        {
            if (wp->nName == 0) /* ignore :noname defs */
                continue;

            cp = wp->name;
            nChars += snprintf(pScr + nChars, sizeof(pVM->scratch) - nChars, "%s", cp);

            if (nChars > 70)
            {
                pScr[nChars] = '\0';
                nChars = 0;
                vmTextOut(pVM, pScr, true);
            }
            else
            {
                len = nCOLWIDTH - nChars % nCOLWIDTH;
                while (len-- > 0)
                    pScr[nChars++] = ' ';
            }

            if (nChars > 70)
            {
                pScr[nChars] = '\0';
                nChars = 0;
                vmTextOut(pVM, pScr, true);
            }
        }
    }

    if (nChars > 0)
    {
        pScr[nChars] = '\0';
        vmTextOut(pVM, pScr, true);
    }

    snprintf(pVM->scratch, sizeof(pVM->scratch), "%s definitions: %d words", pHash->name, nWords);
    vmTextOut(pVM, pVM->scratch, true);
    return;
}


/**************************************************************************
                        l i s t E n v
** Print symbols defined in the environment
**************************************************************************/
static void listEnv(FICL_VM *pVM)
{
    FICL_HASH *pHash = pVM->pSys->envp;
    FICL_WORD *wp;
    unsigned i;
    int nWords = 0;
    char pad[80];

    for (i = 0; i < pHash->size; i++)
    {
        for (wp = pHash->table[i]; wp != NULL; wp = wp->link, nWords++)
        {
            if (wp->opcode == FICL_OP_CONSTANT || wp->opcode == FICL_OP_VALUE)
            {
                snprintf(pad, sizeof(pad), "%s = %i", wp->name, (int)(wp->param[0].i));
                vmTextOut(pVM, pad, true);
            }
            else if (wp->opcode == FICL_OP_2CONSTANT)
            {
                snprintf(pad, sizeof(pad), "%s = %i %i", wp->name, (int)(wp->param[1].i), (int)(wp->param[0].i));
                vmTextOut(pVM, pad, true);
            }
            else
            {
                vmTextOut(pVM, wp->name, true);
            }
        }
    }

    snprintf(pad, sizeof(pad), "Environment: %d words", nWords);
    vmTextOut(pVM, pad, true);
    return;
}


/**************************************************************************
                        e n v C o n s t a n t
** Ficl interface to ficlSetEnv and ficlSetEnvD - allow ficl code to set
** environment constants...
**************************************************************************/
static void envConstant(FICL_VM *pVM)
{
    unsigned value;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 1, 0);
#endif

    vmGetWordToScr(pVM);
    value = POPUNS();
    ficlSetEnv(pVM->pSys, pVM->scratch, (FICL_UNS)value);
    return;
}

static void env2Constant(FICL_VM *pVM)
{
    unsigned v1, v2;

#if FICL_ROBUST > 1
    vmCheckStack(pVM, 2, 0);
#endif

    vmGetWordToScr(pVM);
    v2 = POPUNS();
    v1 = POPUNS();
    ficlSetEnvD(pVM->pSys, pVM->scratch, v1, v2);
    return;
}


/**************************************************************************
                        b u f T e x t O u t
** Replacement OUTFUNC used by >buf.
** Appends text (and optionally a newline) to the null-terminated buffer
** at pVM->pExtend. The caller is responsible for ensuring the buffer
** is large enough.
**************************************************************************/
static void bufTextOut(FICL_VM *pVM, char *text, bool fNewline)
{
    char *buf = (char *)pVM->pExtend;
    if (!buf)
        return;

    char *end = buf + strlen(buf);
    if (text)
    {
        size_t len = strlen(text);
        memcpy(end, text, len);
        end += len;
    }
    if (fNewline)
        *end++ = '\n';
    *end = '\0';
}


/**************************************************************************
                        t o B u f
** >buf ( xt c-addr -- )
** Ficl extra -- Scripted test helper word
** Redirects textOut to the buffer at c-addr, execute xt to completion,
** then restores the previous textOut and pExtend.
** c-addr must point to a writable buffer large enough to hold all output
** produced by xt. The buffer is initialised to empty before xt runs.
**************************************************************************/
static void toBuf(FICL_VM *pVM)
{
    OUTFUNC    savedTextOut = pVM->textOut;
    void      *savedExtend  = pVM->pExtend;
    char      *buf          = (char *)POPPTR();      /* c-addr -- top of stack */
    FICL_WORD *xt           = (FICL_WORD *)POPPTR(); /* xt -- below c-addr */

    if (buf)
        buf[0] = '\0';   /* initialise buffer to empty string */

    pVM->pExtend = buf;
    pVM->textOut = (OUTFUNC) bufTextOut;

    ficlExecXT(pVM, xt);

    pVM->textOut = savedTextOut;
    pVM->pExtend = savedExtend;
}


/**************************************************************************
                        f i c l C o m p i l e T o o l s
** Builds wordset for debugger and TOOLS optional word set
**************************************************************************/

void ficlCompileTools(FICL_SYSTEM *pSys)
{
    FICL_DICT *dp = pSys->dp;
    assert (dp);

    /*
    ** TOOLS and TOOLS EXT
    */
    dictAppendWord(dp, ".s",        displayPStack,  FW_DEFAULT);
    dictAppendWord(dp, "bye",       bye,            FW_DEFAULT);
    dictAppendWord(dp, "forget",    forget,         FW_DEFAULT);
    dictAppendWord(dp, "see",       see,            FW_DEFAULT);
    dictAppendWord(dp, "words",     listWords,      FW_DEFAULT);

    /*
    ** Set TOOLS environment query values
    */
    ficlSetEnv(pSys, "tools",            FICL_TRUE);
    ficlSetEnv(pSys, "tools-ext",        FICL_FALSE);

    /*
    ** Ficl extras
    */
    dictAppendWord(dp, "r.s",       displayRStack,  FW_DEFAULT); /* guy carver */
    dictAppendWord(dp, ".env",      listEnv,        FW_DEFAULT);
    dictAppendWord(dp, "env-constant",
                                    envConstant,    FW_DEFAULT);
    dictAppendWord(dp, "env-2constant",
                                    env2Constant,   FW_DEFAULT);
    dictAppendWord(dp, "debug-xt",  ficlDebugXT,    FW_DEFAULT);
    dictAppendWord(dp, "parse-order",
                                    ficlListParseSteps,
                                                    FW_DEFAULT);
    pSys->pStepBreak =
    dictAppendWord(dp, "step-break",stepBreak,      FW_DEFAULT);
    dictAppendWord(dp, "forget-wid",forgetWid,      FW_DEFAULT);
    dictAppendWord(dp, "see-xt",    seeXT,          FW_DEFAULT);
    dictAppendWord(dp, ">buf",      toBuf,          FW_DEFAULT);

    return;
}
