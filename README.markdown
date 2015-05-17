Arc
===

Introduction
------------

Arc is a simple toy operating system for modern PCs with amd64 processors.
It is written mostly in C11, with small amounts of Intel-style assembly where
required. It can be loaded by any [Multiboot 2][multiboot]-compliant boot
loader, such as [GNU GRUB][grub].

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
  * Scheduling (round-robin, preemptive)
  * System calls (with `SYSCALL`/`SYSRET`)
  * Fine-grained locking with spinlocks

My current short-term goals are:

  * Inter-process communication
  * Some useful system calls (e.g. memory allocation, spawning threads, etc.)
  * Better scheduling algorithm (e.g. MLFQ)

and after that, who knows?

I've also got a cryptic `TODO` list (and some comments spread about the code)
with shorter-term bugs and ideas.

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
licenses used by Arc and GRUB (ISC and GPL respectively) I do not believe that
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

    sudo grub-install --target=i386-pc --root-directory=temp \
      --disk-module=biosdisk --modules="part_msdos ext2" /dev/loop0

Newer versions of GRUB require the following invocation instead:

    sudo grub-install --target=i386-pc --boot-directory=temp/boot \
      --modules="biosdisk part_msdos ext2" /dev/loop0

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

If you mess up the disk image somehow, just delete `disk.img`. The scripts will
automatically create a new disk image from `disk.img.xz`.

From time to time it may be a good idea to run `run/fsck.sh`, which mounts the
disk image and runs `e2fsck` on it, to avoid complaints in the host kernel's log
after the requisite number of mounts have passed.

### Old versions of GRUB

GRUB up to and including version 1.99 has a bug where it load parts of a 64-bit
ELF file as if it were a 32-bit ELF file. Thomas Haller submitted a
[patch to fix this bug][grub-fix] to the GRUB mailing list. If you use one of
the affected versions of GRUB, you'll either need to upgrade to version 2.00 or
above, or apply the patch.

License
-------

Arc is available under the terms of the [ISC license][isc], which is
similar to the 2-clause BSD license. See the `LICENSE` file for the copyright
information and licensing terms.

Arc uses [Doug Lea][dl]'s [memory allocator][dlmalloc] which has been released
into the public domain using the [CC0][cc0] license. See the `LICENSE.dlmalloc`
file for the CC0 text. The CC0 licensing terms only apply to the
`kernel/dlmalloc.h` and `kernel/dlmalloc.c` files.

Finally, whilst not related to licensing, it is worth mentioning the
[OSDev.org wiki][osdev]. It provides a good overview before you jump into the
various manuals and specifications.

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
[vbox]: http://virtualbox.org/
[dl]: http://g.oswego.edu/
[dlmalloc]: http://g.oswego.edu/dl/html/malloc.html
[cc0]: http://creativecommons.org/publicdomain/zero/1.0/
[screenshot]: https://raw.github.com/grahamedgecombe/arc/master/doc/screenshot.png
[osdev]: http://osdev.org/
