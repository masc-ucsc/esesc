#!/bin/bash
NPROC=`nproc`

mkdir /build
cd /build
cmake -DCMAKE_BUILD_TYPE=Release /esesc
make -j${NPROC}
mkdir run
cd run
cp /esesc/conf/*conf* .
cp /esesc/bins/mips64/* .
cp /esesc/bins/inputs/* .
../main/esesc < crafty.in

