#!/bin/bash

# Defaults for configuration variables used by script
: ${ESESC_SRC:=${HOME}/projs/esesc}
: ${ESESC_BUILD_DIR:=${HOME}/build}
: ${ESESC_BUILD_TYPE:=Debug}
: ${ESESC_HOST_PROCS:=`nproc`}
: ${ESESC_ENABLE_LIVE:=0}

BUILD_DIR=${ESESC_BUILD_DIR}/${ESESC_BUILD_TYPE,,}
RUN_DIR=${BUILD_DIR}/run


if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "BUILD ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

if [ -d ${BUILD_DIR} ]; then
  echo "BULID ERROR: '${BUILD_DIR}' already exists"
  exit -2
fi

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=${ESESC_BUILD_TYPE} -DENABLE_LIVE=${ESESC_ENABLE_LIVE} ${ESESC_SRC}
make -j${ESESC_HOST_PROCS}

if [ ${ESESC_ENABLE_LIVE} -eq '1' ]; then
  make live
fi

