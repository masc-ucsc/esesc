#!/bin/bash

# Defaults for configuration variables used by script
: ${ESESC_SRC:=${HOME}/projs/esesc}
: ${ESESC_BUILD_DIR:=${HOME}/build}
: ${ESESC_BUILD_TYPE:=Debug}
: ${ESESC_ENABLE_LIVE:=0}

BENCH_DIR=/bench
BUILD_DIR=${ESESC_BUILD_DIR}/${ESESC_BUILD_TYPE,,}
RUN_DIR=${BUILD_DIR}/run

if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

if [ ! -e ${BUILD_DIR}/main/esesc ]; then
  echo "ERROR: '${BUILD_DIR}' does not contain ESESC binary"
  exit -2
fi

mkdir -p ${RUN_DIR}
cd ${RUN_DIR}
cp ${ESESC_SRC}/conf/*conf* .
cp ${ESESC_SRC}/bins/mips64/* .
cp ${ESESC_SRC}/bins/inputs/* .


# Testing with crafty as default app for now
../main/esesc < crafty.in
#cp /bench/data/spec06/429.mcf/* .
#cp /bench/bins/spec06_mcf.mips64 .
#ls -a
#pwd
#sleep 2
#../main/esesc 

if [ $? -eq '0' ]; then
  ${ESESC_SRC}/conf/scripts/report.pl -last
else
  exit $?
fi

