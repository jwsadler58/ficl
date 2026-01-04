/*
** wasm_main.c
** Minimal Ficl entry points for a browser-based REPL (single VM).
*/
/*
** Usage:
**   make -f makefile.macos wasm
**
** Minimal JS glue (ES module):
**   import createModule from "./ficl_wasm.js";
**   const Module = await createModule();
**   Module._ficl_wasm_init(20000, 256);
**   const linePtr = Module.allocateUTF8("1 2 + .");
**   Module._ficl_wasm_eval(linePtr);
**   Module._free(linePtr);
**   const outPtr = Module._ficl_wasm_get_output();
**   const outLen = Module._ficl_wasm_get_output_len();
**   const output = Module.UTF8ToString(outPtr, outLen);
**   Module._ficl_wasm_clear_output();
**   const stackBuf = Module._malloc(256);
**   Module._ficl_wasm_stack_hex(stackBuf, 256, 8);
**   const stack = Module.UTF8ToString(stackBuf);
**   Module._free(stackBuf);
**   console.log(output, stack);
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ficl.h"

#define WASM_OUTBUF_SIZE 8192

static FICL_SYSTEM *g_sys = NULL;
static FICL_VM *g_vm = NULL;
static char g_output[WASM_OUTBUF_SIZE];
static size_t g_output_len = 0;

void ficlTextOut(FICL_VM *pVM, char *msg, int fNewline)
{
    size_t len;
    size_t remaining;

    (void)pVM;

    if (!msg)
        return;

    len = strlen(msg);
    remaining = (g_output_len < WASM_OUTBUF_SIZE) ? (WASM_OUTBUF_SIZE - g_output_len - 1) : 0;
    if (remaining == 0)
        return;

    if (len > remaining)
        len = remaining;

    memcpy(&g_output[g_output_len], msg, len);
    g_output_len += len;
    g_output[g_output_len] = '\0';

    if (fNewline && g_output_len + 1 < WASM_OUTBUF_SIZE)
    {
        g_output[g_output_len++] = '\n';
        g_output[g_output_len] = '\0';
    }
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

void ficl_wasm_clear_output(void)
{
    g_output_len = 0;
    g_output[0] = '\0';
}

const char *ficl_wasm_get_output(void)
{
    g_output[g_output_len] = '\0';
    return g_output;
}

int ficl_wasm_get_output_len(void)
{
    return (int)g_output_len;
}

int ficl_wasm_init(int dict_cells, int stack_cells)
{
    if (g_sys || g_vm)
        return 0;

    g_sys = ficlInitSystem(dict_cells);
    if (!g_sys)
        return -1;

    if (stack_cells > 0)
        ficlSetStackSize(stack_cells);

    g_vm = ficlNewVM(g_sys);
    if (!g_vm)
        return -2;

    vmSetTextOut(g_vm, ficlTextOut);
    ficl_wasm_clear_output();
    return 0;
}

int ficl_wasm_eval(const char *line)
{
    int ret;

    if (!g_vm || !line)
        return VM_ERREXIT;

    ret = ficlExec(g_vm, (char *)line);
    if (ret == VM_USEREXIT)
        ret = VM_OUTOFTEXT;

    return ret;
}

void ficl_wasm_reset(void)
{
    if (!g_vm)
        return;

    vmReset(g_vm);
}

int ficl_wasm_stack_hex(char *out, int out_len, int max_cells)
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

    if (!g_vm)
        return 0;

    depth = stackDepth(g_vm->pStack);
    count = (max_cells > 0 && max_cells < depth) ? max_cells : depth;

    cursor = out;
    remaining = out_len;

    written = snprintf(cursor, (size_t)remaining, "S:");
    if (written < 0 || written >= remaining)
        return count;

    cursor += written;
    remaining -= written;

    for (i = 0; i < count; ++i)
    {
        CELL cell = stackFetch(g_vm->pStack, i);
        FICL_UNS value = cell.u;
        ultoa(value, hexbuf, 16);
        written = snprintf(cursor, (size_t)remaining, " 0x%s", hexbuf);
        if (written < 0 || written >= remaining)
            break;
        cursor += written;
        remaining -= written;
    }

    return count;
}
