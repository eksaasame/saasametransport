#!/bin/sh
SrcPath=${HOME}/linux2v
systemctl stop linux2v
rpm -e linux2v
InstallPath=/usr/local/linux2v
rm -rf $InstallPath /var/lib/linux2v /var/run/linux2v /etc/linux2v
sh Miniconda3-latest-Linux-x86_64.sh -b -p $InstallPath
$InstallPath/bin/conda update --all -y
$InstallPath/bin/pip install cython ipython circus
$InstallPath/bin/pip install ~/thriftpy
$InstallPath/bin/pip install $SrcPath
$InstallPath/bin/pip install pyramid_ipython
cd $SrcPath/deploy/makerpm
CONDA_PREFIX=$InstallPath sh build.sh
