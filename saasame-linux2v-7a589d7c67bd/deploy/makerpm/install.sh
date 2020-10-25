#!/bin/sh

r=$(lsblk | egrep -i 'lvm' | wc -l)
if [ $r -eq 0 ] || [ -n "`lsblk | grep ssmllncher`" ]; then
  rpm -i bzip2-1.0.6-13.el7.x86_64.rpm
  rpm -i jemalloc-3.6.0-1.el7.x86_64.rpm
  rpm -i redis-2.8.19-2.el7.x86_64.rpm
  systemctl stop linux2v
  rpm -e linux2v
  rpm -i linux2v-*.rpm
else
  echo "No LVM named with ssmllncher, abort Linux Launcher installation."
fi
