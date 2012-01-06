#!/bin/sh
BASEDIR=`dirname $0`
cd $BASEDIR
./image.sh
bochs -qf bochsrc

