#!/bin/sh
BASEDIR=`dirname $0`
cd $BASEDIR
LOOP0=`sudo losetup -f`
sudo losetup $LOOP0 disk.img
LOOP1=`sudo losetup -f`
sudo losetup -o 32256 $LOOP1 $LOOP0
sudo e2fsck -f $LOOP1
sync
sudo losetup -d $LOOP1
sync
sudo losetup -d $LOOP0

