#!/bin/sh
packpath="saasame-linux2v"
packname="saasame-linux2v-`date "+%Y-%m-%dT%H%M%SZ"`.tar.xz"
mkdir $packpath
git archive master | tar -x -C $packpath
rm $packpath/*.ini
rm $packpath/.*
rm -rf $packpath/tests/
rm -rf $packpath/deploy/
tar Jcf $packname $packpath
rm -rf $packpath
mv $packname ~/Pipe/LinuxLauncher/
