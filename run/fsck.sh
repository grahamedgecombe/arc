#!/bin/sh
BASEDIR=`dirname $0`
cd $BASEDIR
LOOP0=`sudo losetup -f --show disk.img`
LOOP1=`sudo losetup -o 1048576 -f --show $LOOP0`
sudo e2fsck -f $LOOP1
sync
sudo losetup -d $LOOP1
sync
sudo losetup -d $LOOP0

