# Ficl 3.6 Release Notes

**Release Date:** February 2026

Ficl 3.6 represents a major evolution of Ficl, featuring significant performance improvements, enhanced floating-point support, robustness enhancements, and enhancements to testmain.

---

## Major Features

### Hybrid Token Threading
**Contributed by Giuseppe Stanghellini**

- 2x-3x performance improvement over subroutine threading
- Implemented token threading optimization using x-macros for performance tuning
- Revised virtual machine instruction dispatch mechanism
- Refactored VM instruction execution (2,326 lines changed in vm.c)

### Floating Point Enhancements

Comprehensive improvements to floating-point support:

- **Non-CELL Float Stack**: Implemented `FICL_FSTACK` for 64-bit float support on 32-bit targets
- **Approximate Float Equality**: Added `f~=` word modeled on Python's `math.isclose()` for better floating-point comparisons
- **Float Environment Variable**: Added `float-epsilon` environment variable for precision control
- **Float Locals Support**: Extended local variable support to include float locals (moved from softwords to jhlocal.fr for proper scoping)
- **Float TO Support**: Completed implementation of `TO` for floats, single cells, double cells, and constants
- **Float Exception Words**: Added floating-point exception handling words
- **Bug Fix**: Fixed `falign` bug in float operations

---

## Interactive Testing Improvements

- **Line Editing**: Added simple line editing capabilities to testmain
- **Test Framework**: Updated standard test file `ttester.fr` adapted for Ficl
- **REPL Improvements**: Exit REPL cleanly on EOF (Ctrl-D) (Periklis Akritidis)

---

## Code Quality and Refactoring

- Replaced all `sprintf` calls with `snprintf` throughout the codebase to prevent buffer overrun vulnerabilities
- Changed `ultoa`, `ltoa`, `strrev` to `ficlLtoa`, `ficlUltoa`, `ficlStrrev` to avoid naming conflicts with system libraries
- Removed duplicate and dead code - now committed to hybrid token threading

---

## Build and Platform Improvements
### Conditional Compilation
- `FICL_WANT_FLOAT` sets `FICL_WANT_LOCAL` as well. Thank me later.
- FICL_ scalar and float types are now auto-sized unless you override, simplifying porting.
- Float size auto-selected or you can select 64 bit floats in a 32 bit machine. (Giouseppe Stanghellini)
- Known-width data types now use stdint.h (C11)

### Platform Support
- **WASM Support**: Asyncified demo.html and updated wasm_main.c
- **Makefile Updates**: Added minimal target to makefile.macos
- **EOL Standardization**: Converted entire codebase to Unix EOL sequences

### Environment Words
- `.env` now prints values of environment parameters

---

## Bug Fixes

- Fixed `falign` bug in float operations
- Fixed `sprintf` buffer overflow vulnerability in line editor
- Fixed output overwrite issue in testmain
- Fixed `FICL_WANT_FLOAT` conditional compilation issues
- testmain now exits with Ctrl-D (Periklis Akritidis)
- Fixed sprintf buffer overflow issues (Periklis Akritidis)
- Removed sscanf() from the code to reduce lib dependency
- Fixed a comment bug in ficlSprintf

---

## Upgrading from 3.5

This release maintains backward compatibility for most use cases. Notable changes that may affect existing code:

1. **Float Stack**: If using `FICL_WANT_FLOAT`, 32-bit targets may now elect 64 bit floats.
2. **Function Names**: `ultoa`, `ltoa`, and `strrev` are now `ficlUltoa`, `ficlUltoa`, and `ficlStrrev`
3. **Float Locals**: `FICL_WANT_FLOAT` now implies `FICL_WANT_LOCAL`. Float stack words are not as rich as scalar stack words, so local variables are very helpful.

---

## Acknowledgments

Special thanks to:
- **Giuseppe Stanghellini** for contributing hybrid token threading and enhanced float support.
- **Periklis Akriditis** for bug fixes and helpful suggestions.

---

## Links

- Git Tag: `ficl306`
- Previous Release: `ficl305` (Version 3.05)
