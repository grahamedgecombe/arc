#!/bin/sh
if [ ! -f disk.img ]; then
  if [ ! -f disk.img.lzma ]; then
    echo "No disk image template. See README.markdown for more information." >&2
    exit 1
  fi
  lzcat disk.img.lzma > disk.img
fi
mkdir -p mnt
sudo mount -o loop,offset=32256 disk.img mnt
sudo cp ../arc mnt/boot
sudo cp grub.cfg mnt/boot/grub
sync
sudo umount mnt

