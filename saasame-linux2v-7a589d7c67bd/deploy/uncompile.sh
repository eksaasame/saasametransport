#!/bin/sh
systemctl stop linux2v
pip uninstall linux2v -y
find linux2v -name '*.so' | xargs rm
rm -rf build dist
git pull
git reset --hard

