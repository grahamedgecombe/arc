Arc
===

Introduction
------------

Arc is a simple hobby operating system for modern PCs with amd64
processors. It is written mostly in C99, with small amounts of Intel-style
assembly where required.

Building
--------

The following software is required to build Arc:

  * [Clang][clang] or [GCC][gcc].
  * [GNU Binutils][binutils].
  * [NASM][nasm] or [YASM][yasm].
  * [GNU Make][make].

GCC and Binutils must be cross-compiled for the `x86_64-pc-elf` target.

When the required software is installed, simply use the `make` command to build
the operating system.

Testing
-------

The easiest way to test Arc is with the [QEMU][qemu] or [Bochs][bochs]
emulators. Simply type `./run/qemu.sh` or `./run/bochs.sh` to launch QEMU or
Bochs respectively.

License
-------

Arc is available under the terms of the [ISC license][isc], which is a
similar to the 2-clause BSD license.

[clang]: http://clang.llvm.org/
[gcc]: http://gcc.gnu.org/
[binutils]: http://gnu.org/software/binutils/
[nasm]: http://nasm.us/
[yasm]: http://yasm.tortall.net/
[make]: http://gnu.org/software/make/
[qemu]: http://qemu.org/
[bochs]: http://bochs.sourceforge.net/
[isc]: http://isc.org/software/license/

