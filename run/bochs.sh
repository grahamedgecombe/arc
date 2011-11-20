#!/bin/sh -e
BASEDIR=`dirname $0`
cd $BASEDIR
./image.sh
bochs -qf bochsrc

