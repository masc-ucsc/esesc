#!/bin/bash

if [ "$#" -lt 1 ]; then
  echo "Usage: <esesc_src_dir> [build_type] [enable_live] [docker_image]"
  exit -1
fi

ESESC_SRC=$1
BUILD_TYPE=${2:-Debug}
ENABLE_LIVE=${3:-0}
DOCKER_IMAGE=${4:-mascucsc/esescbase}

: ${ESESC_HOST_PROCS:=`nproc`}

DOCKER_ESESC_SRC='/esesc'
DOCKER_BUILD_DIR='/build'

if [ ! -e ${ESESC_SRC}/CMakeLists.txt ]; then
  echo "BUILD ERROR: '${ESESC_SRC}' does not contain ESESC source code"
  exit -1
fi

docker run  -t \
  -v $ESESC_SRC:$DOCKER_ESESC_SRC:ro \
  -e ESESC_SRC=${DOCKER_ESESC_SRC} \
  -e ESESC_BUILD_DIR=${BUILD_DIR} \
  -e ESESC_BUILD_TYPE=${BUILD_TYPE} \
  -e ESESC_HOST_PROCS=${ESESC_HOST_PROCS} \
  -e ESESC_ENABLE_LIVE=${ENABLE_LIVE} \
  ${DOCKER_IMAGE} /esesc/conf/scripts/build-and-run.sh 

