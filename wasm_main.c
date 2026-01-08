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
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
    if (fNewline && nOutbuf < WASM_OUTBUF_SIZE - 1)
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

/* For web demo - emaulate a block of LEDs */
static int fakeLED = 0;
EM_JS(void, ficlWasmSetLedBits, (int value), {
    if (typeof window !== "undefined" && typeof window.setLedBits === "function")
        window.setLedBits(value | 0);
});

/* and call some javascript to change LED visibility*/
static void setFakeLed(FICL_VM *pVM)
{
    fakeLED = stackPopINT(pVM->pStack);
    ficlWasmSetLedBits(fakeLED);
    return;
}


int ficlWasmInit(int dict_cells, int stack_cells)
{
    if (pSys || pVm)
        return 0;

    pSys = ficlInitSystem(dict_cells);
    if (!pSys)
        return -1;
    
    ficlBuild(pSys, "!led", setFakeLed, FW_DEFAULT);

    if (stack_cells > 0)
        ficlSetStackSize(stack_cells);

    pVm = ficlNewVM(pSys);
    if (!pVm)
        return -2;

    ficlWasmClearOutput();
    (void)ficlEvaluate(pVm, ".ver 2 spaces .( " __DATE__ " ) cr");
    return 0;
}

int ficlWasmEval(const char *line)
{
    int ret;

    if (!pVm || !line)
        return VM_ERREXIT;

    ret = ficlExec(pVm, (char *)line);
    if (ret == VM_USEREXIT)   /* ignore 'bye' */
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
    int radix;
    char *cursor;
    char numbuf[32];

    if (!out || out_len <= 0)
        return 0;

    out[0] = '\0';

    if (!pVm)
        return 0;

    radix = pVm->base;
    depth = stackDepth(pVm->pStack);
    count = (max_cells > 0 && max_cells < depth) ? max_cells : depth;

    cursor = out;
    remaining = out_len;

    written = snprintf(cursor, (size_t)remaining, "%d deep", depth);
    if (written < 0 || written >= remaining)
        return count;

    cursor += written;
    remaining -= written;

    for (i = 0; i < count; ++i)
    {
        CELL cell = stackFetch(pVm->pStack, i);
        FICL_UNS value = cell.u;
        ultoa(value, numbuf, radix);
        written = snprintf(cursor, (size_t)remaining, "\n%s", numbuf);
        if (written < 0 || written >= remaining)
            break;
        cursor += written;
        remaining -= written;
    }

    return count;
}
