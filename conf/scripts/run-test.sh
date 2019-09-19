#!/bin/bash

# Defaults for configuration variables used by script
: ${ESESC_SRC:=${HOME}/projs/esesc}
: ${ESESC_BUILD_DIR:=${HOME}/build}
: ${ESESC_BUILD_TYPE:=Debug}
: ${ESESC_TARGET:riscv64}
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
cp ${ESESC_SRC}/bins/${ESESC_TARGET}/* .
cp ${ESESC_SRC}/bins/inputs/* .

export ESESC_BenchName="spec00_crafty.${ESESC_TARGET}"
export ESESC_TASS_nInstSkip=1e8
export ESESC_TASS_nInstMax=2e8
export ESESC_samplerSel="TASS"

# Testing with crafty as default app for now
../main/esesc < crafty.in
if [ $? -eq 0 ]; then
  ${ESESC_SRC}/conf/scripts/report.pl -last
else
  exit $?
fi

