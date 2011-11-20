#!/bin/sh -e
BASEDIR=`dirname $0`
cd $BASEDIR
./image.sh
qemu-system-x86_64 -monitor stdio -hda disk.img

