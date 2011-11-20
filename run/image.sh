#!/bin/sh -e
if [ ! -f disk.img ]; then
  lzcat disk.img.lzma > disk.img
fi
mkdir -p mnt
sudo mount -o loop,offset=32256 disk.img mnt
sudo cp ../arc mnt/boot
sudo cp grub.cfg mnt/boot/grub
sync
sudo umount mnt

