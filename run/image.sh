#!/bin/sh
if [ ! -f disk.img ]; then
  if [ ! -f disk.img.xz ]; then
    echo "No disk image template. See README.markdown for more information." >&2
    exit 1
  fi
  xzcat disk.img.xz > disk.img
fi
mkdir -p mnt
sudo mount -o loop,offset=1048576 disk.img mnt
sudo mkdir -p mnt/bin
sudo cp ../kernel/vmarc mnt/boot
sudo cp ../hello/hello mnt/bin
sudo cp grub.cfg mnt/boot/grub
sync
sudo umount mnt

