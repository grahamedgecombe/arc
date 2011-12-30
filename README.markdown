Arc
===

Introduction
------------

Arc is a simple hobby operating system for modern PCs with amd64
processors. It is written mostly in C11, with small amounts of Intel-style
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

To use these scripts you must create a [GNU GRUB][grub] disk image. Due to the
licenses used by Arc and GRUB, this image cannot be distributed with the Arc
code.

### Generating `disk.img.lzma`

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
then type "1" to make it the first. Leave all the other settings as the default
by simply hitting the return key. Finally use "w" to save the changes.

Now set up a loop device for the partition:

    sudo losetup -o 32256 /dev/loop1 /dev/loop0

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

The template is compressed using LZMA to save space (most of it is full of
zeroes), to compress it run the following command:

    lzma -9 disk.img

The should create a `disk.img.lzma` file. Put this in the `run` folder of the
Arc distribution. The `run/qemu.sh` and `run/bochs.sh` scripts should now work
assuming you have the correct software installed and Arc was compiled
correctly.

### GRUB Patch

Some versions of GRUB have a bug, and try to load parts of a 64-bit ELF file as
if it were a 32-bit ELF file. If booting with your distribution's GRUB package
doesn't work, or if you downloaded GRUB from source, you will need to change it
slightly to fix this problem.

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
[diff and some more information][grub-fix].

License
-------

Arc is available under the terms of the [ISC license][isc], which is a
similar to the 2-clause BSD license. See the `LICENSE` file for the copyright
information and licensing terms.

[clang]: http://clang.llvm.org/
[gcc]: http://gcc.gnu.org/
[binutils]: http://gnu.org/software/binutils/
[nasm]: http://nasm.us/
[yasm]: http://yasm.tortall.net/
[make]: http://gnu.org/software/make/
[qemu]: http://qemu.org/
[bochs]: http://bochs.sourceforge.net/
[isc]: http://isc.org/software/license/
[grub]: http://gnu.org/software/grub/
[grub-fix]: http://lists.gnu.org/archive/html/bug-grub/2011-09/msg00026.html

