#!/bin/sh
BASEDIR=`dirname $0`
cd $BASEDIR
sudo losetup /dev/loop0 disk.img
sudo losetup -o 32256 /dev/loop1 /dev/loop0
sudo e2fsck -f /dev/loop1
sync
sudo losetup -d /dev/loop1
sudo losetup -d /dev/loop0

