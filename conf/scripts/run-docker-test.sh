#!/bin/bash

if [ "$#" -lt 1 ]; then
  echo "Usage: <esesc_src_dir> <benchmarks> [build_type] [enable_live] [docker_image]"
  exit -1
fi

ESESC_SRC=$1
BENCH_SRC=$2
LOCAL_BUILD=$3
BUILD_TYPE=${4:-Debug}
ENABLE_LIVE=${5:-0}
DOCKER_IMAGE=${6:-mascucsc/esescbase}


: ${ESESC_HOST_PROCS:=`nproc`}

DOCKER_ESESC_SRC='/esesc'
BUILD_DIR='/build'
DOCKER_BENCH_DIR='/bench'

if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "BUILD ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

# possibly add back -t command later
docker run  \
  -v $ESESC_SRC:$DOCKER_ESESC_SRC \
  -v $BENCH_SRC:$DOCKER_BENCH_DIR \
  -v $LOCAL_BUILD:${BUILD_DIR} \
  -e ESESC_SRC=${DOCKER_ESESC_SRC} \
  -e ESESC_BUILD_DIR=${BUILD_DIR} \
  -e ESESC_BUILD_TYPE=${BUILD_TYPE} \
  -e ESESC_HOST_PROCS=${ESESC_HOST_PROCS} \
  -e ESESC_ENABLE_LIVE=${ENABLE_LIVE} \
  ${DOCKER_IMAGE} /esesc/conf/scripts/build-and-run.sh 

