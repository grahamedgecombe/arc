#!/bin/sh
BASEDIR=`dirname $0`
cd $BASEDIR
./image.sh
if [ -f disk.vdi ]; then
  VBoxManage unregistervm arc --delete
  rm -f disk.vdi
fi
VBoxManage convertfromraw disk.img disk.vdi
VBoxManage createvm --name arc --register
VBoxManage modifyvm arc --acpi on --ioapic on --cpus 2 --memory 128 --boot1 disk --boot2 none --boot3 none --boot4 none
VBoxManage storagectl arc --name "SATA Controller" --add sata
VBoxManage storageattach arc --storagectl "SATA Controller" --port 0 --type hdd --medium disk.vdi
VBoxManage startvm arc
