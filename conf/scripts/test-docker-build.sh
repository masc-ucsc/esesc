#!/bin/bash

ESESC_DIR=/esesc
BUILD_DIR=/build


NPROC=`nproc`

if [ ! -d ${ESESC_DIR} ]; then
  exit -1
fi

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=Release ${ESESC_DIR}
make -j${NPROC}
mkdir run
cd run
cp ${ESESC_DIR}/conf/*conf* .
cp ${ESESC_DIR}/bins/mips64/* .
cp ${ESESC_DIR}/bins/inputs/* .
../main/esesc < crafty.in

