#!/usr/bin/bash
#-*-coding:utf-8 -*-
set -ex
# prepare environment
mkdir -p build
export LC_ALL="C"
export LIBRARY_PATH=/usr/local/lib:/usr/lib64:$LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:/usr/lib64:$LIBRARY_PATH
# build
cd build && cmake .. && make
cd ..
# run
./ogre
#sudo mknod mem -m666 c 1 1
