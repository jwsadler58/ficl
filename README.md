Ficl is a lightweight, embeddable scripting language designed to be incorporated into other programs, and especially embedded systems that may have memory and OS constraints. Applications include scripting, hardware bring-up, rapid prototyping, and system extensions.

Unlike Lua or Python, Ficl acts as a component of your system - you feed it stuff to do, it does the stuff, and comes back to you for more. You can export compiled code to Ficl, execute Ficl code from your compiled code, or interact. Your choice.

### Notes
1. Ficl is written portably in Standard C (11) without special compiler features.
2. Ficl includes a compact object model that can wrap compiled code and data structures.
3. Ficl conforms to the 1994 ANSI Standard for Forth

____________
## More information
### Current release: Ficl306, February 2026
### The [official Ficl website](https://ficl.sourceforge.net/)
### Get the [latest Ficl release](https://sourceforge.net/projects/ficl/)
### Ficl [Github Repo](https://github.com/jwsadler58/ficl)

____________
## INSTALLATION

Ficl builds out-of-the-box on MacOS, Linux, and Windows.
To port to other platforms, examine and revise **sysdep.h** as needed. We suggest you start with the Linux makefile.

____________
## FICL LICENSE (BSD 2.0)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS
IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


