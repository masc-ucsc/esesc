#!/bin/bash

# Defaults for configuration variables used by script
: ${ESESC_SRC:=${HOME}/projs/esesc}
: ${ESESC_BUILD_DIR:=${HOME}/build}
: ${ESESC_BUILD_TYPE:=Debug}
: ${ESESC_TARGET:riscv64}
: ${ESESC_HOST_PROCS:=$(nproc)}
: ${ESESC_ENABLE_LIVE:=0}

BUILD_DIR=${ESESC_BUILD_DIR}/${ESESC_BUILD_TYPE,,}
RUN_DIR=${BUILD_DIR}/run

case ${ESESC_TARGET} in
    riscv)
        ESESC_MIPSR6=0
        ;;
    mips64)
        ESESC_MIPSR6=1
        ;;
esac

if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "BUILD ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

if [ -d ${BUILD_DIR} ]; then
  if [ -e ${BUILD_DIR}/Makefile ]; then
    cd ${BUILD_DIR}
    make -j${ESESC_HOST_PROCS}
  else
    echo "BULID ERROR: '${BUILD_DIR}' already exists"
    exit -2
  fi
else
  mkdir -p ${BUILD_DIR}
  cd ${BUILD_DIR}
  cmake -DCMAKE_BUILD_TYPE=${ESESC_BUILD_TYPE} -DENABLE_LIVE=${ESESC_ENABLE_LIVE} -DENABLE_LIVE=${ESESC_ENABLE_LIVECRIU} -DESESC_MIPSR6=${ESESC_MIPSR6} ${ESESC_SRC}
  make -j${ESESC_HOST_PROCS}
fi

if [ ${ESESC_ENABLE_LIVE} -eq '1' ]; then
  make live
  make livecriu
fi

