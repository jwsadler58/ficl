/*******************************************************************
                    s y s d e p . h
** Forth Inspired Command Language
** Author: John W Sadler
** Created: 16 Oct 1997
** Ficl system dependent types and prototypes...
**
** Note: Ficl depends on the use of "assert" when
** FICL_ROBUST is enabled. This may require some consideration
** in firmware systems since assert often
** assumes stderr/stdout.
** $Id: sysdep.h,v 1.11 2001-11-11 12:25:46-08 jsadler Exp jsadler $
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

#if !defined (__SYSDEP_H__)
#define __SYSDEP_H__

#include <stddef.h> /* size_t, NULL */
#include <setjmp.h>
#include <assert.h>
#include <limits.h> /* CHAR_BIT, UCHAR_MAX */
#include <stdint.h> /* int32_t, uint32_t, uint16_t, uint8_t */
#include <stdalign.h>

/*
** A Ficl CELL must be wide enough to contain a (void *) pointer, an unsigned, or an int.
** I assume here that the pointer is register width
** This is guarded by static_asserts in sysdep.c
**
** CELL_ALIGN is the power of two to which the dictionary
** pointer address must be aligned. This value is usually
** either 2 or 3, depending on the memory architecture
** of the target system; 2 is safe on any 16 or 32 bit
** machine. 3 would be appropriate for a 64 bit machine.
*/
enum {
    CELL_BYTES = sizeof(void *),
    CELL_BITS = CELL_BYTES * CHAR_BIT,
    CELL_ALIGN = (CELL_BYTES == 4) ? 2 :
                 (CELL_BYTES == 8) ? 3 :
                 (CELL_BYTES == 16) ? 4 :
                 -1,  // unsupported values
    CELL_ALIGN_ADD = CELL_BYTES - 1
};

/*
** Ficl presently supports values of 32 and 64 for CELL_BITS
*/
static_assert(CHAR_BIT == 8, "Unsupported for non-byte chars");
static_assert(CELL_ALIGN > 0, "Unsupported CELL_BITS value");



/************************************************************************************
**         P L A T F O R M - S P E C I F I C   D E F I N I T I O N S
*************************************************************************************/

/*
**         MAC OSX Sequoia 15.4.x (so far)
*/
#if defined(__APPLE__)
#include <TargetConditionals.h>
    #if TARGET_OS_OSX
        #define FICL_INT long
        #define FICL_UNS unsigned long
        #define FICL_HAVE_FTRUNCATE 1

        #define MACOS
    #elif TARGET_OS_IOS
        /* iOS only */
    #endif  /* TARGET_OS */
#endif  /* __APPLE__ */


/*
**         Linux  (Skip Carter)
*/
#if defined(linux)
    #define FICL_HAVE_FTRUNCATE 1
#endif

/*
**         FreeBSD Alpha (64 bit) data types
*/
#if defined (FREEBSD_ALPHA)
    #define INT32 int
    #define UNS32 unsigned int
    #define FICL_INT long
    #define FICL_UNS unsigned long
#endif

/*
**         Windows - Sprintf format tags to silence MSVC warnings
*/
#if defined(_WIN32)
    #define PCT_LD     "%Id"
    #define PCT_IX      "%#Ix"
#else
    #define PCT_LD     "%ld"
    #define PCT_IX      "%#lx"
#endif



/************************************************************************************
**         E N D  P L A T F O R M - S P E C I F I C   D E F I N I T I O N S
*************************************************************************************/

/*
** IGNORE Macro to silence "unused parameter" warnings
*/
#if !defined IGNORE
    #define IGNORE(x) &x
#endif

/*
** TRUE and FALSE for C boolean operations, and
** portable 32 bit types for CELLs
**
*/
#if !defined TRUE
    #define TRUE 1
#endif
#if !defined FALSE
    #define FALSE 0
#endif

/************************************************************************************
**         B U I L D   C O N T R O L S
*************************************************************************************/

#if !defined (FICL_MINIMAL)
#define FICL_MINIMAL 0
#endif
#if (FICL_MINIMAL)
#define FICL_WANT_SOFTWORDS  0
#define FICL_WANT_FILE       0
#define FICL_WANT_FLOAT      0
#define FICL_WANT_USER       0
#define FICL_WANT_LOCALS     0
#define FICL_WANT_DEBUGGER   0
#define FICL_WANT_OOP        0
#define FICL_PLATFORM_EXTEND 0
#define FICL_MULTISESSION    0
#define FICL_ROBUST          0
#define FICL_EXTENDED_PREFIX 0
#define FICL_UNIT_TEST       0
#endif


/*
** FICL_PLATFORM_EXTEND
** Includes words defined in ficlCompilePlatform (see unix.c for example)
*/
#if !defined (FICL_PLATFORM_EXTEND)
#define FICL_PLATFORM_EXTEND 0
#endif


/*
** FICL_WANT_FILE
** Includes the FILE and FILE-EXT wordset and associated code. Turn this off if you do not
** have a file system!
** Contributed by Larry Hastings
*/
#if !defined (FICL_WANT_FILE)
#define FICL_WANT_FILE 1
#endif

/*
** FICL_WANT_FLOAT
** Includes a floating point stack for the VM, and words to do float and float extension operations.
** Contributed by Giuseppe Stanghellini and Guy Carver
*/
#if !defined (FICL_WANT_FLOAT)
#define FICL_WANT_FLOAT 1
#endif


/*
** FICL_WANT_DEBUGGER
** Inludes a simple source level debugger
*/
#if !defined (FICL_WANT_DEBUGGER)
#define FICL_WANT_DEBUGGER 1
#endif

/*
** FICL_EXTENDED_PREFIX enables a bunch of extra prefixes in prefix.c and prefix.fr (if
** included as part of softcore.c)
*/
#if !defined FICL_EXTENDED_PREFIX
#define FICL_EXTENDED_PREFIX 0
#endif

/*
** FICL_WANT_OOP
** Inludes object oriented programming support (in softwords)
** OOP support requires locals and user variables!
*/
#if !defined (FICL_WANT_OOP)
    #define FICL_WANT_OOP 1
#endif

#if (FICL_WANT_OOP)
    #if !defined (FICL_WANT_LOCALS)
        #define FICL_WANT_LOCALS 1
    #else
        #undef FICL_WANT_LOCALS
        #define FICL_WANT_LOCALS 1
    #endif
    #if !defined (FICL_WANT_USER)
        #define FICL_WANT_USER 1
    #else
        #undef FICL_WANT_USER
        #define FICL_WANT_USER 1
    #endif
#endif

/*
** User variables: per-instance variables bound to the VM.
** Kinda like thread-local storage. Could be implemented in a
** VM private dictionary, but I've chosen the lower overhead
** approach of an array of CELLs instead.
*/
#if !defined FICL_WANT_USER
#define FICL_WANT_USER 1
#endif

#if !defined FICL_USER_CELLS
#define FICL_USER_CELLS 16
#endif

/*
** FICL_WANT_LOCALS controls the creation of the LOCALS wordset and
** a private dictionary for local variable compilation.
*/
#if !defined FICL_WANT_LOCALS
#define FICL_WANT_LOCALS 1
#endif

/*
** Max number of local variables per definition
** Standard requires at least 16
** 2locals require 2 cells each
** flocals may require 1 or 2 cells each depending on float size
*/
#if !defined FICL_MAX_LOCALS
#define FICL_MAX_LOCALS 32
#endif

/*
** FICL_WANT_SOFTWORDS
** Controls inclusion of all softwords in softcore.c
*/
#if !defined (FICL_WANT_SOFTWORDS)
#define FICL_WANT_SOFTWORDS 1
#endif

/*
** DEPRECATED: FICL_MULTISESSION
** This has never been implemented to my knowledge.
** FICL_MULTISESSION enables dictionary mutual exclusion
** wia the ficlLockDictionary system dependent function.
** Note: experimental. Necessary only if you intend to support
** multiple threads per FICL_SYSTEM more than one of which may modify the dictionary.
*/
#if !defined FICL_MULTISESSION
#define FICL_MULTISESSION 0
#endif

/*
** PORTABLE_LONGMULDIV causes ficlLongMul and ficlLongDiv to be defined in C in sysdep.c.
** If set to 0, you will need to implement them.
*/
#if !defined (PORTABLE_LONGMULDIV)
#define PORTABLE_LONGMULDIV 1
#endif

/*
** FICL_SUBR_THREADING causes the inner interpreter to use subroutine threading at all times.
** If this is 0 (default), we will use hybrid token threading instead for speed.
*/
#if !defined FICL_SUBR_THREADING
#define FICL_SUBR_THREADING 0
#endif

/*
** FICL_ROBUST enables bounds checking of stacks and the dictionary.
** This will detect stack over and underflows and dictionary overflows.
** Any exceptional condition will result in an assertion failure.
** (As generated by the ANSI assert macro)
** FICL_ROBUST == 1 --> stack checking in the outer interpreter
** FICL_ROBUST == 2 also enables checking in many primitives
*/

#if !defined FICL_ROBUST
#define FICL_ROBUST 1
#endif

/*
** FICL_DEFAULT_STACK Specifies the default size (in CELLs) of
** a new virtual machine's stacks, unless overridden at
** create time.
*/
#if !defined FICL_DEFAULT_STACK
#define FICL_DEFAULT_STACK 256
#endif

/*
** FICL_DEFAULT_DICT specifies the number of CELLs to allocate
** for the system dictionary by default. The value
** can be overridden at startup time as well.
** FICL_DEFAULT_ENV specifies the number of cells to allot
** for the environment-query dictionary.
*/
#if !defined FICL_DEFAULT_DICT
#define FICL_DEFAULT_DICT 12288
#endif

#if !defined FICL_DEFAULT_ENV
#define FICL_DEFAULT_ENV 512
#endif

/*
** FICL_DEFAULT_VOCS specifies the maximum number of wordlists in
** the dictionary search order. See Forth DPANS sec 16.3.3
** (file://dpans16.htm#16.3.3)
*/
#if !defined FICL_DEFAULT_VOCS
#define FICL_DEFAULT_VOCS 16
#endif

/*
** FICL_MAX_PARSE_STEPS controls the size of an array in the FICL_SYSTEM structure
** that stores pointers to parser extension functions. I would never expect to have
** more than 8 of these, so that's the default limit. Too many of these functions
** will probably exact a performance penalty.
*/
#if !defined FICL_MAX_PARSE_STEPS
#define FICL_MAX_PARSE_STEPS 8
#endif

/*
** Known-width data type declarations - now using stdint.h (C11)
*/
#if !defined INT32
    typedef int32_t INT32;
#endif

#if !defined UNS32
    typedef uint32_t UNS32;
#endif

#if !defined UNS16
    typedef uint16_t UNS16;
#endif

#if !defined UNS8
    typedef uint8_t UNS8;
#endif

static_assert(sizeof(INT32) * CHAR_BIT == 32, "INT32 must be 32 bits");
static_assert(sizeof(UNS32) * CHAR_BIT == 32, "UNS32 must be 32 bits");

/*
** FICL_UNS and FICL_INT must be the same size as a void* on
** the target system. Static asserts below enforce this.
** A CELL is a union of void*, FICL_UNS, and FICL_INT.
*/
#if !defined FICL_INT
    typedef intptr_t FICL_INT;
#endif

#if !defined FICL_UNS
    typedef uintptr_t FICL_UNS;
#endif

static_assert(sizeof(FICL_INT) == sizeof(void *), "FICL_INT must match pointer size");
static_assert(sizeof(FICL_UNS) == sizeof(void *), "FICL_UNS must match pointer size");

#if FICL_WANT_FLOAT
    /*
    ** FICL_FLOAT is auto-sized to match pointer size by default.
    ** FICL_FLOAT_BITS may be set to 32 or 64 to override auto float size selection.
    ** FICL_FLOATs have their own stack, and need not be the same size as CELLs.
    */
    #if defined FICL_FLOAT_BITS
        #if (FICL_FLOAT_BITS == 32)
            #define FICL_FLOAT float
            #define FICL_FLT_MAX FLT_MAX /* float.h */
            #define FICL_FLOAT_EPSILON FLT_EPSILON
        #elif (FICL_FLOAT_BITS == 64)
            #define FICL_FLOAT double
            #define FICL_FLT_MAX DBL_MAX /* float.h */
            #define FICL_FLOAT_EPSILON DBL_EPSILON
        #else
            #error FICL_FLOAT_BITS must be 32 or 64
        #endif
    #else           /* auto-select float size to match pointer size */
        #if INTPTR_MAX == INT64_MAX
            #define FICL_FLOAT double
            #define FICL_FLT_MAX DBL_MAX /* float.h */
            #define FICL_FLOAT_EPSILON DBL_EPSILON
            #define FICL_FLOAT_BITS 64
        #elif INTPTR_MAX == INT32_MAX
            #define FICL_FLOAT float
            #define FICL_FLT_MAX FLT_MAX /* float.h */
            #define FICL_FLOAT_EPSILON FLT_EPSILON
            #define FICL_FLOAT_BITS 32
        #else
            #error Unsupported pointer size for FICL_FLOAT
        #endif
    #endif

    #if defined FICL_FLOAT_BITS
        static_assert((FICL_FLOAT_BITS == 32) || (FICL_FLOAT_BITS == 64),
                      "FICL_FLOAT_BITS must be 32 or 64");
        static_assert(sizeof(FICL_FLOAT) * CHAR_BIT == FICL_FLOAT_BITS,
                      "FICL_FLOAT_BITS does not match FICL_FLOAT size");
    #endif
    #define FICL_FLOAT_ALIGN_BYTES ((size_t)alignof(FICL_FLOAT))
    #define FICL_FLOAT_ALIGN_MASK  (FICL_FLOAT_ALIGN_BYTES - 1)
    static_assert(FICL_FLOAT_ALIGN_BYTES > 0, "FICL_FLOAT_ALIGN_BYTES must be non-zero");
#endif /* FICL_WANT_FLOAT */


typedef struct
{
    FICL_UNS hi;
    FICL_UNS lo;
} DPUNS;

typedef struct
{
    FICL_UNS quot;
    FICL_UNS rem;
} UNSQR;

typedef struct
{
    FICL_INT hi;
    FICL_INT lo;
} DPINT;

typedef struct
{
    FICL_INT quot;
    FICL_INT rem;
} INTQR;


/*
** System dependent routines --
** Edit the implementations in sysdep.c to fit your runtime environment.
**
** ficlTextOut sends a NULL terminated string to the
**   default output device - used for system error messages
** ficlMalloc and ficlFree have the same semantics as malloc and free
**   in standard C
** ficlLongMul multiplies two UNS32s and returns a 64 bit unsigned
**   product
** ficlLongDiv divides an UNS64 by an UNS32 and returns UNS32 quotient
**   and remainder
*/
struct vm;
void  ficlTextOut(struct vm *pVM, char *msg, int fNewline);
void *ficlMalloc (size_t size);
void  ficlFree   (void *p);
void *ficlRealloc(void *p, size_t size);

/*
** DEPRECATED Stub function for dictionary access control
** Does nothing by default.
** Redefine to guarantee exclusive dict access to a single thread for updates.
** All dict update code must be bracketed as follows:
** ficlLockDictionary(TRUE);
** <code that updates dictionary>
** ficlLockDictionary(FALSE);
**
** Returns zero if successful, nonzero if unable to acquire lock
** before timeout (optional - could also block forever)
**
** NOTE: this function must be implemented with lock counting
** semantics so that nested calls behave properly.
*/
#if FICL_MULTISESSION
int ficlLockDictionary(short fLock);
#else
#define ficlLockDictionary(x) 0 /* ignore */
#endif

/*
** Double precision integer math support routines: multiply two FICL_UNS
** to get a double width product, & divide the product by a FICL_UNS
** to get an FICL_UNS quotient and remainder.
** These are implemented in sysdep.c by default using portable C code.
*/
DPUNS ficlLongMul(FICL_UNS x, FICL_UNS y);
UNSQR ficlLongDiv(DPUNS    q, FICL_UNS y);


/*
** FICL_HAVE_FTRUNCATE indicates whether the current OS supports
** the ftruncate() function (available on most UNIXes).  This
** function is necessary to provide the complete File-Access wordset.
*/
#if !defined (FICL_HAVE_FTRUNCATE)
#define FICL_HAVE_FTRUNCATE 1
#endif

/*
** Unit test extensions based on Unity from ThrowTheSwitch.org
*/
#if !defined FICL_UNIT_TEST
    #define FICL_UNIT_TEST 1
#endif


#endif /*__SYSDEP_H__*/
