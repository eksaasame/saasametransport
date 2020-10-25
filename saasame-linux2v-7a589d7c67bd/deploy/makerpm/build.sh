#!/bin/sh
# Note: psutil, persistent, zodbpickle compiles C ext. thriftpy uses cython.
rm SOURCES/bundle-*
mkdir -p {BUILD,RPMS,SOURCES,SPECS,SRPMS}
if [ -n "${CONDA_PREFIX}" ]; then
  ${CONDA_PREFIX}/bin/conda package --pkg-name linux2v
else
  conda package --pkg-name linux2v
fi
cp Miniconda3-latest-Linux-x86_64.sh BUILD/
cd SOURCES && tar Jcf linux2v-etc.tar.xz linux2v.ini linux2v.service circus.ini
cd ..
mv linux2v-*.tar.bz2 SOURCES/bundle-latest.tar.bz2
if [ -e SOURCES/bundle-latest.tar.bz2 ];then
  rpmbuild -bb linux2v.spec
else
  echo "Error: bundle-latest.tar.bz2 not found."
  exit 1
fi
