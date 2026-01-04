/*
** wasm_main.c
** Minimal Ficl entry points for a browser-based REPL (single VM)
*/
/*
** Usage:
**   make -f makefile.macos wasm
**
** Minimal JS glue (ES module):
**   import createModule from "./ficl_wasm.js";
**   const Module = await createModule();
**   Module._ficlWasmInit(20000, 256);
**   const line = "1 2 + .";
**   const len = Module.lengthBytesUTF8(line) + 1;
**   const base = Module.stackSave();
**   const linePtr = Module.stackAlloc(len);
**   Module.stringToUTF8(line, linePtr, len);
**   Module._ficlWasmEval(linePtr);
**   Module.stackRestore(base);
**   const outPtr = Module._ficlWasmGetOutput();
**   const outLen = Module._ficlWasmGetOutputLen();
**   const output = outLen ? Module.UTF8ToString(outPtr, outLen) : "";
**   Module._ficlWasmClearOutput();
**   const stackBufSize = 256;
**   const stackBase = Module.stackSave();
**   const stackBuf = Module.stackAlloc(stackBufSize);
**   Module._ficlWasmStackHex(stackBuf, stackBufSize, 8);
**   const stack = Module.UTF8ToString(stackBuf);
**   Module.stackRestore(stackBase);
**   console.log(output, stack);
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ficl.h"

#define WASM_OUTBUF_SIZE 8192

/* 
** Static pointers to the system and vm
*/
static FICL_SYSTEM *pSys = NULL;
static FICL_VM *pVm = NULL;
static char outbuf[WASM_OUTBUF_SIZE];
static size_t nOutbuf = 0;


void ficlTextOut(FICL_VM *pVM, char *msg, int fNewline)
{
    IGNORE(pVM);

    if (!msg)
        return;

    while (nOutbuf < WASM_OUTBUF_SIZE - 1 && *msg)
    {
        outbuf[nOutbuf++] = *msg++;
    }
    if (fNewline && nOutbuf + 1 < WASM_OUTBUF_SIZE)
    {
        outbuf[nOutbuf++] = '\n';
    }
    outbuf[nOutbuf] = '\0';
    return;
}

void *ficlMalloc(size_t size)
{
    return malloc(size);
}

void *ficlRealloc(void *p, size_t size)
{
    return realloc(p, size);
}

void ficlFree(void *p)
{
    free(p);
}

void ficlWasmClearOutput(void)
{
    nOutbuf = 0;
    outbuf[0] = '\0';
}

const char *ficlWasmGetOutput(void)
{
    outbuf[nOutbuf] = '\0';
    return outbuf;
}

int ficlWasmGetOutputLen(void)
{
    return (int)nOutbuf;
}

int ficlWasmInit(int dict_cells, int stack_cells)
{
    if (pSys || pVm)
        return 0;

    pSys = ficlInitSystem(dict_cells);
    if (!pSys)
        return -1;

    if (stack_cells > 0)
        ficlSetStackSize(stack_cells);

    pVm = ficlNewVM(pSys);
    if (!pVm)
        return -2;

    ficlWasmClearOutput();
    (void)ficlEvaluate(pVm, ".ver .( " __DATE__ " ) cr quit");
    return 0;
}

int ficlWasmEval(const char *line)
{
    int ret;

    if (!pVm || !line)
        return VM_ERREXIT;

    ret = ficlExec(pVm, (char *)line);
    if (ret == VM_USEREXIT)
        ret = VM_OUTOFTEXT;

    return ret;
}

void ficlWasmReset(void)
{
    if (!pVm)
        return;

    vmReset(pVm);
}

int ficlWasmStackHex(char *out, int out_len, int max_cells)
{
    int depth;
    int count;
    int i;
    int written;
    int remaining;
    char *cursor;
    char hexbuf[32];

    if (!out || out_len <= 0)
        return 0;

    out[0] = '\0';

    if (!pVm)
        return 0;

    depth = stackDepth(pVm->pStack);
    count = (max_cells > 0 && max_cells < depth) ? max_cells : depth;

    cursor = out;
    remaining = out_len;

    written = snprintf(cursor, (size_t)remaining, "n = %d", depth);
    if (written < 0 || written >= remaining)
        return count;

    cursor += written;
    remaining -= written;

    for (i = 0; i < count; ++i)
    {
        CELL cell = stackFetch(pVm->pStack, i);
        FICL_UNS value = cell.u;
        ultoa(value, hexbuf, 16);
        written = snprintf(cursor, (size_t)remaining, "\n0x%s", hexbuf);
        if (written < 0 || written >= remaining)
            break;
        cursor += written;
        remaining -= written;
    }

    return count;
}
