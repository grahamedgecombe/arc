#!/bin/sh -e
BASEDIR=`dirname $0`
cd $BASEDIR
./image.sh
qemu-system-x86_64 -m 128 -monitor stdio -hda disk.img

