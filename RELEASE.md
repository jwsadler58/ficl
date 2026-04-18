# Ficl 3.065 Release Notes

**Release Date:** 18 April 2026

Cleanups and an optional ISR-safe way of interrupting the inner interpreter with a small overhead penalty.
Cleanups contributed by François Perrad
---

## Cleanups
* Fix many gcc warnings, part of -Wall & -Wextra:-Wformat (on target 32bits)-Wimplicit-fallthrough-Wmisleading-indentation-Wshift-negative-value-Wstrict-prototypes-Wunused-function-Wunused-value-Wunused-variable-Wwrite-string (g++)
* Constify a lot of char* variables & parameters
* Refactor with the c99 bool
* Sanitize many macros with parentheses
* Unit tests (unity) moved out the ficl library
* Fix with conditional compilation with FICL_WANT_LOCAL & FICL_WANT_USER
* Other minor code cleanup
* Makefiles:
    * add makefile.wasm
    * minor fixes

## Interrupts
* vmSigint (renamed from vmInterrupt)
* vmInterrupt (now safe for calling from an ISR)

## Upgrading

You should not see any effects other than an absence of compiler warnings when moving to this release from a recent Ficl release.

---

## Acknowledgments

Special thanks to:
- **François Perrad** for contributing many cleanup improvements

---

## Links

- Git Tag: `ficl3065`
- Previous Release: `ficl306` (Version 3.06)
