# Instructions for Ficl development
- Follow the style of the existing codebase in naming, indentation, capitalization. See details below.

## Ficl .c and .h file conventions
- Indentation
  - Allman style - curly braces on their own lines at same indent as the contol structure keyword
- Function Naming:
  - camelCase
  - first three letters reference the module name. Example: dpmath.c function names begin with dpm.
- Macro naming
  - ALL CAPS for constants
  - Underscores between words
  - Function-like macros are camelCased

- Comments
  - Comment block before each exported function
  - Comment block at the head of the file including the license and disclaimer. Match the style
 of the project.
  - Keep inline comments brief
  - Align hanging comments to column 32

## javascript code (WASM)
- camelCase function names
- block comment before each function or class
- align hanging comments to column 32


## Testing your work
### Ficl C and Forth files
- Make both ficl and ficlmin targets from makefile.macos, and check for compile errors and warnings.
- Run ficl --test to execute unit and scripted tests. Any errors will be reported to stdout, and the process will exit with a nonzero code.
### WASM demo
- Make the wasm target and check for errors or warnings in the build
- Propose unit tests when I ask you to propose code changes

### Test automation
  - Create automated tests as companions to any new development.
  - Review existing test coverage before undertaking refactoring or redesign unless the changes are provably correct.
  - Unit tests use the unity test framework and are invoked from testmain.c. Give preference to locating unit tests for a given function in the same  source file.
  - Scripted tests are in the test/ subdirectory. Testmain invokes scripted tests by changing to the test/ directory and loading ficltest.fr. This in turn loads ttester.fr and standard forth2012 test scripts, then conducts additional tests using the t{ }t framework defined in ttesster.fr.


## planning and doing code
- Always begin a coding assignment with a proposal including at least two options
- Once I have selected an option, develop it into a detailed design
- Review affected code for cruft, inconsistencies, and opportunities to improve readability

## answering questions
- no need to give options unless you are explicitly asked to do so
- if the question is ambiguous, ask a clarifying question
- When you use an acronym for the first time, explain what it means

