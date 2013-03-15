Arc
===

Introduction
------------

Arc is a simple hobby operating system for modern PCs with amd64
processors. It is written mostly in C11, with small amounts of Intel-style
assembly where required. It can be loaded by any
[Multiboot 2][multiboot]-compliant boot loader, such as [GNU GRUB][grub].

Screenshot
----------

![Arc running the 'hello' program][screenshot]

Features
--------

The current feature set, at a high level, is roughly:

  * Symmetric multiprocessing (i.e. multiple processors/cores)
  * Paging (and TLB shootdown)
  * Physical memory allocation (several zones for DMA addressing limitations)
  * Virtual memory allocation (in user- and kernel-space)
  * Interrupt handling (dual 8259 PICs or local APIC)
  * Interrupt routing (with I/O APIC and ACPI tables)
  * Timing (8253/8254 PIT or local APIC)
  * Processes (loaded as ELF64 Multiboot modules) and threads
  * Scheduling (round-robin)
  * System calls (with `SYSCALL`/`SYSRET`)
  * Fine-grained locking with spinlocks

My current short-term goals are:

  * Idle thread per CPU so processes can terminate
  * Inter-process communication
  * Some useful system calls (e.g. memory allocation, spawning threads, etc.)
  * Better scheduling algorithm (e.g. MLFQ)

and after that, who knows?

Building
--------

The following software is required to build Arc:

  * [GCC][gcc] or [Clang][clang]
  * [GNU Binutils][binutils]
  * [NASM][nasm] or [YASM][yasm]
  * [GNU Make][make]

GCC and Binutils must be [cross-compiled][cross] for the `x86_64-pc-elf` target.

When the required software is installed, simply use the `make` command to build
the operating system.

Testing
-------

The easiest way to test Arc is with the [QEMU][qemu], [Bochs][bochs] or
[VirtualBox][vbox] emulators. Simply type `./run/qemu.sh`, `./run/bochs.sh` or
`./run/virtualbox.sh` to launch QEMU, Bochs or VirtualBox respectively.

To use these scripts you must create a [GNU GRUB][grub] disk image. Due to the
licenses used by Arc and GRUB (GPL and ISC respectively) I do not believe that
this image can be distributed with the Arc code.

### Generating `disk.img.xz`

To create this image you will need to install a recent version of GRUB 2, e.g.
from your Linux distribution's package manager. You could also compile and
install the code yourself but that is beyond the scope of this guide.

First you should create an empty image file and set up a loop device for it:

    dd if=/dev/zero of=disk.img bs=512 count=32130
    sudo losetup /dev/loop0 disk.img

Then run fdisk:

    sudo fdisk /dev/loop0

At the fdisk prompt, first type "o" to create a new partition table. Then type
"n" to create a new partition. Type "p" to set it as the primary partition and
then type "1" to make it the first. The first sector should be set to 2048, this
is probably already the default. Leave all the other settings as the default
by simply hitting the return key. Finally use "w" to save the changes.

Now set up a loop device for the partition:

    sudo losetup -o 1048576 /dev/loop1 /dev/loop0

Format this partition as ext2 and mount it to a temporary location:

    sudo mke2fs /dev/loop1
    mkdir temp
    sudo mount /dev/loop1 temp

Install GRUB to the partition:

    sudo grub-install --root-directory=temp --disk-module=biosdisk \
      --modules="part_msdos ext2" /dev/loop0

Finally unmount the partition, remove the loop devices and remove the temporary
mount point:

    sudo umount temp
    sudo losetup -d /dev/loop1
    sudo losetup -d /dev/loop0
    rmdir temp

The template is compressed using XZ to save space (most of it is full of
zeroes), to compress it run the following command:

    xz -9 disk.img

The should create a `disk.img.xz` file. Put this in the `run` folder of the
Arc distribution. The `run/{qemu,bochs,virtualbox}.sh` scripts should now work
assuming you have the correct software installed and Arc was compiled
correctly.

### Patching GRUB

GRUB up to version 1.99 has a bug where it load parts of a 64-bit ELF file as
if it were a 32-bit ELF file. If booting with your distribution's GRUB package
doesn't work, or if you downloaded GRUB 1.99 from source, you will need to
patch it to fix this problem.

In `grub-core/loader/multiboot_elfxx.c` you should add the following:

    # define Elf_Shdr Elf32_Shdr

After:

    # define Elf_Phdr Elf32_Phdr

Likewise add:

    # define Elf_Shdr Elf64_Shdr

After:

    # define Elf_Phdr Elf64_Phdr

Finally add:

    #undef Elf_Shdr

After:

    #undef Elf_Phdr

Thomas Haller, who submitted this fix to the GRUB mailing list, also provided a
[diff and some more information][grub-fix]. It is also in the
[Bazaar repository][grub-fix-bzr] and therefore this step won't be necessary
with future versions of GRUB. I believe it is no longer necessary in GRUB 2.00
and above.

License
-------

Arc is available under the terms of the [ISC license][isc], which is a
similar to the 2-clause BSD license. See the `LICENSE` file for the copyright
information and licensing terms.

Arc uses [Doug Lea][dl]'s [memory allocator][dlmalloc] which has been released
into the public domain using the [CC0][cc0] license. See the `LICENSE.dlmalloc`
file for the CC0 text. The CC0 licensing terms only apply to the
`include/dlmalloc.h` and `src/dlmalloc.c` files.

[multiboot]: http://download.savannah.gnu.org/releases/grub/phcoder/multiboot.pdf
[clang]: http://clang.llvm.org/
[gcc]: http://gcc.gnu.org/
[binutils]: http://gnu.org/software/binutils/
[nasm]: http://nasm.us/
[yasm]: http://yasm.tortall.net/
[make]: http://gnu.org/software/make/
[cross]: http://wiki.osdev.org/GCC_Cross-Compiler
[qemu]: http://qemu.org/
[bochs]: http://bochs.sourceforge.net/
[isc]: http://isc.org/software/license/
[grub]: http://gnu.org/software/grub/
[grub-fix]: http://lists.gnu.org/archive/html/bug-grub/2011-09/msg00026.html
[grub-fix-bzr]: http://bzr.savannah.gnu.org/lh/grub/trunk/grub/revision/3427
[vbox]: http://virtualbox.org/
[dl]: http://g.oswego.edu/
[dlmalloc]: http://g.oswego.edu/dl/html/malloc.html
[cc0]: http://creativecommons.org/publicdomain/zero/1.0/
[screenshot]: https://raw.github.com/grahamedgecombe/arc/master/doc/screenshot.png
